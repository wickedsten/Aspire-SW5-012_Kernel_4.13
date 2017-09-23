/*
 * VirtualBox Guest Shared Folders support: Utility functions.
 * Mainly conversion from/to VirtualBox/Linux data structures.
 *
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <linux/namei.h>
#include <linux/nfs_fs.h>
#include <linux/nls.h>
#include <linux/vfs.h>
#include <linux/vbox_err.h>
#include <linux/vbox_utils.h>
#include "vfsmod.h"

/*
 * sf_reg_aops and sf_backing_dev_info are just quick implementations to make
 * sendfile work. For more information have a look at
 *
 *   http://us1.samba.org/samba/ftp/cifs-cvs/ols2006-fs-tutorial-smf.odp
 *
 * and the sample implementation
 *
 *   http://pserver.samba.org/samba/ftp/cifs-cvs/samplefs.tar.gz
 */

static void sf_timespec_from_vbox(struct timespec *tv,
				  const struct shfl_timespec *ts)
{
	s64 nsec, t = ts->ns_relative_to_unix_epoch;

	nsec = do_div(t, 1000000000);
	tv->tv_sec = t;
	tv->tv_nsec = nsec;
}

static void sf_timespec_to_vbox(struct shfl_timespec *ts,
				const struct timespec *tv)
{
	s64 t = (s64) tv->tv_nsec + (s64) tv->tv_sec * 1000000000;

	ts->ns_relative_to_unix_epoch = t;
}

/* set [inode] attributes based on [info], uid/gid based on [sf_g] */
void sf_init_inode(struct sf_glob_info *sf_g, struct inode *inode,
		   const struct shfl_fsobjinfo *info)
{
	const struct shfl_fsobjattr *attr;
	s64 allocated;
	int mode;

	attr = &info->Attr;

#define mode_set(r) attr->fMode & (SHFL_UNIX_##r) ? (S_##r) : 0;

	mode = mode_set(ISUID);
	mode |= mode_set(ISGID);

	mode |= mode_set(IRUSR);
	mode |= mode_set(IWUSR);
	mode |= mode_set(IXUSR);

	mode |= mode_set(IRGRP);
	mode |= mode_set(IWGRP);
	mode |= mode_set(IXGRP);

	mode |= mode_set(IROTH);
	mode |= mode_set(IWOTH);
	mode |= mode_set(IXOTH);

#undef mode_set

	inode->i_mapping->a_ops = &sf_reg_aops;

	if (SHFL_IS_DIRECTORY(attr->fMode)) {
		inode->i_mode = sf_g->dmode != ~0 ? (sf_g->dmode & 0777) : mode;
		inode->i_mode &= ~sf_g->dmask;
		inode->i_mode |= S_IFDIR;
		inode->i_op = &sf_dir_iops;
		inode->i_fop = &sf_dir_fops;
		/* XXX: this probably should be set to the number of entries
		   in the directory plus two (. ..) */
		set_nlink(inode, 1);
	} else if (SHFL_IS_SYMLINK(attr->fMode)) {
		inode->i_mode = sf_g->fmode != ~0 ? (sf_g->fmode & 0777) : mode;
		inode->i_mode &= ~sf_g->fmask;
		inode->i_mode |= S_IFLNK;
		inode->i_op = &sf_lnk_iops;
		set_nlink(inode, 1);
	} else {
		inode->i_mode = sf_g->fmode != ~0 ? (sf_g->fmode & 0777) : mode;
		inode->i_mode &= ~sf_g->fmask;
		inode->i_mode |= S_IFREG;
		inode->i_op = &sf_reg_iops;
		inode->i_fop = &sf_reg_fops;
		set_nlink(inode, 1);
	}

	inode->i_uid = make_kuid(current_user_ns(), sf_g->uid);
	inode->i_gid = make_kgid(current_user_ns(), sf_g->gid);

	inode->i_size = info->cbObject;
	inode->i_blkbits = 12;
	/* i_blocks always in units of 512 bytes! */
	allocated = info->cbAllocated + 511;
	do_div(allocated, 512);
	inode->i_blocks = allocated;

	sf_timespec_from_vbox(&inode->i_atime, &info->AccessTime);
	sf_timespec_from_vbox(&inode->i_ctime, &info->ChangeTime);
	sf_timespec_from_vbox(&inode->i_mtime, &info->ModificationTime);
}

int sf_stat(const char *caller, struct sf_glob_info *sf_g,
	    SHFLSTRING * path, struct shfl_fsobjinfo *result, int ok_to_fail)
{
	struct shfl_createparms params = {};
	int rc;

	params.Handle = SHFL_HANDLE_NIL;
	params.CreateFlags = SHFL_CF_LOOKUP | SHFL_CF_ACT_FAIL_IF_NEW;
	rc = vboxsf_create(sf_g->root, path, &params);
	if (rc == VERR_INVALID_NAME) {
		/* this can happen for names like 'foo*' on a Windows host */
		return -ENOENT;
	}
	if (rc < 0)
		return -EPROTO;

	if (params.Result != SHFL_FILE_EXISTS)
		return -ENOENT;

	*result = params.Info;
	return 0;
}

/* this is called directly as iop on 2.4, indirectly as dop
   [sf_dentry_revalidate] on 2.4/2.6, indirectly as iop through
   [sf_getattr] on 2.6. the job is to find out whether dentry/inode is
   still valid. the test is failed if [dentry] does not have an inode
   or [sf_stat] is unsuccessful, otherwise we return success and
   update inode attributes */
int sf_inode_revalidate(struct dentry *dentry)
{
	int err;
	struct sf_glob_info *sf_g;
	struct sf_inode_info *sf_i;
	struct shfl_fsobjinfo info;

	if (!dentry || !dentry->d_inode)
		return -EINVAL;

	sf_g = GET_GLOB_INFO(dentry->d_inode->i_sb);
	sf_i = GET_INODE_INFO(dentry->d_inode);

	BUG_ON(!sf_g);
	BUG_ON(!sf_i);

	if (!sf_i->force_restat) {
		if (jiffies - dentry->d_time < sf_g->ttl)
			return 0;
	}

	err = sf_stat(__func__, sf_g, sf_i->path, &info, 1);
	if (err)
		return err;

	dentry->d_time = jiffies;
	sf_init_inode(sf_g, dentry->d_inode, &info);
	return 0;
}

/* this is called during name resolution/lookup to check if the
   [dentry] in the cache is still valid. the job is handled by
   [sf_inode_revalidate] */
static int sf_dentry_revalidate(struct dentry *dentry, unsigned flags)
{
	if (flags & LOOKUP_RCU)
		return -ECHILD;

	if (sf_inode_revalidate(dentry))
		return 0;

	return 1;
}

int sf_getattr(const struct path *path, struct kstat *kstat, u32 request_mask,
	       unsigned int flags)
{
	int err;
	struct dentry *dentry = path->dentry;

	err = sf_inode_revalidate(dentry);
	if (err)
		return err;

	generic_fillattr(dentry->d_inode, kstat);
	return 0;
}

int sf_setattr(struct dentry *dentry, struct iattr *iattr)
{
	struct sf_glob_info *sf_g;
	struct sf_inode_info *sf_i;
	struct shfl_createparms params = {};
	struct shfl_fsobjinfo info = {};
	uint32_t cbBuffer;
	int rc, err;

	sf_g = GET_GLOB_INFO(dentry->d_inode->i_sb);
	sf_i = GET_INODE_INFO(dentry->d_inode);
	err = 0;

	params.Handle = SHFL_HANDLE_NIL;
	params.CreateFlags = SHFL_CF_ACT_OPEN_IF_EXISTS
	    | SHFL_CF_ACT_FAIL_IF_NEW | SHFL_CF_ACCESS_ATTR_WRITE;

	/* this is at least required for Posix hosts */
	if (iattr->ia_valid & ATTR_SIZE)
		params.CreateFlags |= SHFL_CF_ACCESS_WRITE;

	rc = vboxsf_create(sf_g->root, sf_i->path, &params);
	if (rc < 0) {
		err = vbg_status_code_to_errno(rc);
		goto fail2;
	}
	if (params.Result != SHFL_FILE_EXISTS) {
		err = -ENOENT;
		goto fail1;
	}
#define mode_set(r) ((iattr->ia_mode & (S_##r)) ? SHFL_UNIX_##r : 0)

	/*
	 * Setting the file size and setting the other attributes has to
	 * be handled separately.
	 */
	if (iattr->ia_valid & (ATTR_MODE | ATTR_ATIME | ATTR_MTIME)) {
		if (iattr->ia_valid & ATTR_MODE) {
			info.Attr.fMode = mode_set(ISUID);
			info.Attr.fMode |= mode_set(ISGID);
			info.Attr.fMode |= mode_set(IRUSR);
			info.Attr.fMode |= mode_set(IWUSR);
			info.Attr.fMode |= mode_set(IXUSR);
			info.Attr.fMode |= mode_set(IRGRP);
			info.Attr.fMode |= mode_set(IWGRP);
			info.Attr.fMode |= mode_set(IXGRP);
			info.Attr.fMode |= mode_set(IROTH);
			info.Attr.fMode |= mode_set(IWOTH);
			info.Attr.fMode |= mode_set(IXOTH);

			if (iattr->ia_mode & S_IFDIR)
				info.Attr.fMode |= SHFL_TYPE_DIRECTORY;
			else
				info.Attr.fMode |= SHFL_TYPE_FILE;
		}

		if (iattr->ia_valid & ATTR_ATIME)
			sf_timespec_to_vbox(&info.AccessTime, &iattr->ia_atime);
		if (iattr->ia_valid & ATTR_MTIME)
			sf_timespec_to_vbox(&info.ModificationTime,
					    &iattr->ia_mtime);
		/* ignore ctime (inode change time) as it can't be set from userland anyway */

		cbBuffer = sizeof(info);
		rc = vboxsf_fsinfo(sf_g->root, params.Handle,
				   SHFL_INFO_SET | SHFL_INFO_FILE, &cbBuffer,
				   (PSHFLDIRINFO) & info);
		if (rc < 0) {
			err = vbg_status_code_to_errno(rc);
			goto fail1;
		}
	}
#undef mode_set

	if (iattr->ia_valid & ATTR_SIZE) {
		memset(&info, 0, sizeof(info));
		info.cbObject = iattr->ia_size;
		cbBuffer = sizeof(info);
		rc = vboxsf_fsinfo(sf_g->root, params.Handle,
				   SHFL_INFO_SET | SHFL_INFO_SIZE, &cbBuffer,
				   (PSHFLDIRINFO) & info);
		if (rc < 0) {
			err = vbg_status_code_to_errno(rc);
			goto fail1;
		}
	}

	vboxsf_close(sf_g->root, params.Handle);

	return sf_inode_revalidate(dentry);

fail1:
	vboxsf_close(sf_g->root, params.Handle);
fail2:
	return err;
}

static int sf_make_path(const char *caller, struct sf_inode_info *sf_i,
			const char *d_name, size_t d_len, SHFLSTRING ** result)
{
	size_t path_len, shflstring_len;
	SHFLSTRING *tmp;
	uint16_t p_len;
	uint8_t *p_name;
	int fRoot = 0;

	p_len = sf_i->path->u16Length;
	p_name = sf_i->path->String.utf8;

	if (p_len == 1 && *p_name == '/') {
		path_len = d_len + 1;
		fRoot = 1;
	} else {
		/* lengths of constituents plus terminating zero plus slash  */
		path_len = p_len + d_len + 2;
		if (path_len > 0xffff)
			return -ENAMETOOLONG;
	}

	shflstring_len = offsetof(SHFLSTRING, String.utf8) + path_len;
	tmp = kmalloc(shflstring_len, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	tmp->u16Length = path_len - 1;
	tmp->u16Size = path_len;

	if (fRoot)
		memcpy(&tmp->String.utf8[0], d_name, d_len + 1);
	else {
		memcpy(&tmp->String.utf8[0], p_name, p_len);
		tmp->String.utf8[p_len] = '/';
		memcpy(&tmp->String.utf8[p_len + 1], d_name, d_len);
		tmp->String.utf8[p_len + 1 + d_len] = '\0';
	}

	*result = tmp;
	return 0;
}

/**
 * [dentry] contains string encoded in coding system that corresponds
 * to [sf_g]->nls, we must convert it to UTF8 here and pass down to
 * [sf_make_path] which will allocate SHFLSTRING and fill it in
 */
int sf_path_from_dentry(const char *caller, struct sf_glob_info *sf_g,
			struct sf_inode_info *sf_i, struct dentry *dentry,
			SHFLSTRING ** result)
{
	int err;
	const char *d_name;
	size_t d_len;
	const char *name;
	size_t len = 0;

	d_name = dentry->d_name.name;
	d_len = dentry->d_name.len;

	if (sf_g->nls) {
		size_t in_len, i, out_bound_len;
		const char *in;
		char *out;

		in = d_name;
		in_len = d_len;

		out_bound_len = PATH_MAX;
		out = kmalloc(out_bound_len, GFP_KERNEL);
		name = out;

		for (i = 0; i < d_len; ++i) {
			wchar_t uni;
			int nb;

			nb = sf_g->nls->char2uni(in, in_len, &uni);
			if (nb < 0) {
				err = -EINVAL;
				goto fail1;
			}
			in_len -= nb;
			in += nb;

			nb = utf32_to_utf8(uni, out, out_bound_len);
			if (nb < 0) {
				err = -EINVAL;
				goto fail1;
			}
			out_bound_len -= nb;
			out += nb;
			len += nb;
		}
		if (len >= PATH_MAX - 1) {
			err = -ENAMETOOLONG;
			goto fail1;
		}

		*out = 0;
	} else {
		name = d_name;
		len = d_len;
	}

	err = sf_make_path(caller, sf_i, name, len, result);
	if (name != d_name)
		kfree(name);

	return err;

fail1:
	kfree(name);
	return err;
}

int sf_nlscpy(struct sf_glob_info *sf_g,
	      char *name, size_t name_bound_len,
	      const unsigned char *utf8_name, size_t utf8_len)
{
	if (sf_g->nls) {
		const char *in;
		char *out;
		size_t out_len;
		size_t out_bound_len;
		size_t in_bound_len;

		in = utf8_name;
		in_bound_len = utf8_len;

		out = name;
		out_len = 0;
		out_bound_len = name_bound_len;

		while (in_bound_len) {
			int nb;
			unicode_t uni;

			nb = utf8_to_utf32(in, in_bound_len, &uni);
			if (nb < 0)
				return -EINVAL;

			in += nb;
			in_bound_len -= nb;

			nb = sf_g->nls->uni2char(uni, out, out_bound_len);
			if (nb < 0)
				return nb;

			out += nb;
			out_bound_len -= nb;
			out_len += nb;
		}

		*out = 0;
	} else {
		if (utf8_len + 1 > name_bound_len)
			return -ENAMETOOLONG;

		memcpy(name, utf8_name, utf8_len + 1);
	}
	return 0;
}

static struct sf_dir_buf *sf_dir_buf_alloc(void)
{
	struct sf_dir_buf *b;

	b = kmalloc(sizeof(*b), GFP_KERNEL);
	if (!b)
		return NULL;

	b->buf = kmalloc(DIR_BUFFER_SIZE, GFP_KERNEL);
	if (!b->buf) {
		kfree(b);
		return NULL;
	}

	INIT_LIST_HEAD(&b->head);
	b->cEntries = 0;
	b->cbUsed = 0;
	b->cbFree = DIR_BUFFER_SIZE;
	return b;
}

static void sf_dir_buf_free(struct sf_dir_buf *b)
{
	BUG_ON(!b || !b->buf);

	list_del(&b->head);
	kfree(b->buf);
	kfree(b);
}

/**
 * Free the directory buffer.
 */
void sf_dir_info_free(struct sf_dir_info *p)
{
	struct list_head *list, *pos, *tmp;

	list = &p->info_list;
	list_for_each_safe(pos, tmp, list) {
		struct sf_dir_buf *b;

		b = list_entry(pos, struct sf_dir_buf, head);
		sf_dir_buf_free(b);
	}
	kfree(p);
}

/**
 * Empty (but not free) the directory buffer.
 */
void sf_dir_info_empty(struct sf_dir_info *p)
{
	struct list_head *list, *pos, *tmp;
	list = &p->info_list;
	list_for_each_safe(pos, tmp, list) {
		struct sf_dir_buf *b;
		b = list_entry(pos, struct sf_dir_buf, head);
		b->cEntries = 0;
		b->cbUsed = 0;
		b->cbFree = DIR_BUFFER_SIZE;
	}
}

/**
 * Create a new directory buffer descriptor.
 */
struct sf_dir_info *sf_dir_info_alloc(void)
{
	struct sf_dir_info *p;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return NULL;

	INIT_LIST_HEAD(&p->info_list);
	return p;
}

/**
 * Search for an empty directory content buffer.
 */
static struct sf_dir_buf *sf_get_empty_dir_buf(struct sf_dir_info *sf_d)
{
	struct list_head *list, *pos;

	list = &sf_d->info_list;
	list_for_each(pos, list) {
		struct sf_dir_buf *b;

		b = list_entry(pos, struct sf_dir_buf, head);
		if (!b)
			return NULL;
		else {
			if (b->cbUsed == 0)
				return b;
		}
	}

	return NULL;
}

int sf_dir_read_all(struct sf_glob_info *sf_g, struct sf_inode_info *sf_i,
		    struct sf_dir_info *sf_d, SHFLHANDLE handle)
{
	int err;
	SHFLSTRING *mask;
	struct sf_dir_buf *b;

	err = sf_make_path(__func__, sf_i, "*", 1, &mask);
	if (err)
		goto fail0;

	for (;;) {
		int rc;
		void *buf;
		uint32_t cbSize;
		uint32_t cEntries;

		b = sf_get_empty_dir_buf(sf_d);
		if (!b) {
			b = sf_dir_buf_alloc();
			if (!b) {
				err = -ENOMEM;
				goto fail1;
			}
			list_add(&b->head, &sf_d->info_list);
		}

		buf = b->buf;
		cbSize = b->cbFree;

		rc = vboxsf_dirinfo(sf_g->root, handle, mask, 0, 0,
				    &cbSize, buf, &cEntries);
		switch (rc) {
		case VINF_SUCCESS:
			/* fallthrough */
		case VERR_NO_MORE_FILES:
			break;
		case VERR_NO_TRANSLATION:
			/* XXX */
			break;
		default:
			err = vbg_status_code_to_errno(rc);
			goto fail1;
		}

		b->cEntries += cEntries;
		b->cbFree -= cbSize;
		b->cbUsed += cbSize;

		if (rc < 0)
			break;
	}
	err = 0;

fail1:
	kfree(mask);

fail0:
	return err;
}

int sf_get_volume_info(struct super_block *sb, struct kstatfs *stat)
{
	struct sf_glob_info *sf_g;
	SHFLVOLINFO SHFLVolumeInfo;
	u32 cbBuffer;
	int rc;

	sf_g = GET_GLOB_INFO(sb);
	cbBuffer = sizeof(SHFLVolumeInfo);
	rc = vboxsf_fsinfo(sf_g->root, 0,
			   SHFL_INFO_GET | SHFL_INFO_VOLUME, &cbBuffer,
			   (PSHFLDIRINFO)&SHFLVolumeInfo);
	if (rc < 0)
		return vbg_status_code_to_errno(rc);

	stat->f_type = NFS_SUPER_MAGIC;	/* XXX vboxsf type? */
	stat->f_bsize = SHFLVolumeInfo.ulBytesPerAllocationUnit;

	do_div(SHFLVolumeInfo.ullTotalAllocationBytes,
	       SHFLVolumeInfo.ulBytesPerAllocationUnit);
	stat->f_blocks = SHFLVolumeInfo.ullTotalAllocationBytes;

	do_div(SHFLVolumeInfo.ullAvailableAllocationBytes,
	       SHFLVolumeInfo.ulBytesPerAllocationUnit);
	stat->f_bfree  = SHFLVolumeInfo.ullAvailableAllocationBytes;
	stat->f_bavail = SHFLVolumeInfo.ullAvailableAllocationBytes;

	stat->f_files = 1000;
	stat->f_ffree = 1000;	/* don't return 0 here since the guest may think
				 * that it is not possible to create any more files */
	stat->f_fsid.val[0] = 0;
	stat->f_fsid.val[1] = 0;
	stat->f_namelen = 255;
	return 0;
}

struct dentry_operations sf_dentry_ops = {
	.d_revalidate = sf_dentry_revalidate
};
