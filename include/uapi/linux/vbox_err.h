/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __UAPI_VBOX_ERR_H__
#define __UAPI_VBOX_ERR_H__

/**
 * @name VirtualBox virtual-hardware error macros
 * @{
 */

#define VINF_SUCCESS                        0
#define VERR_GENERAL_FAILURE                (-1)
#define VERR_INVALID_PARAMETER              (-2)
#define VERR_INVALID_MAGIC                  (-3)
#define VERR_INVALID_HANDLE                 (-4)
#define VERR_LOCK_FAILED                    (-5)
#define VERR_INVALID_POINTER                (-6)
#define VERR_IDT_FAILED                     (-7)
#define VERR_NO_MEMORY                      (-8)
#define VERR_ALREADY_LOADED                 (-9)
#define VERR_PERMISSION_DENIED              (-10)
#define VERR_VERSION_MISMATCH               (-11)
#define VERR_NOT_IMPLEMENTED                (-12)
#define VERR_INVALID_FLAGS                  (-13)

#define VERR_NOT_EQUAL                      (-18)
#define VERR_NOT_SYMLINK                    (-19)
#define VERR_NO_TMP_MEMORY                  (-20)
#define VERR_INVALID_FMODE                  (-21)
#define VERR_WRONG_ORDER                    (-22)
#define VERR_NO_TLS_FOR_SELF                (-23)
#define VERR_FAILED_TO_SET_SELF_TLS         (-24)
#define VERR_NO_CONT_MEMORY                 (-26)
#define VERR_NO_PAGE_MEMORY                 (-27)
#define VERR_THREAD_IS_DEAD                 (-29)
#define VERR_THREAD_NOT_WAITABLE            (-30)
#define VERR_PAGE_TABLE_NOT_PRESENT         (-31)
#define VERR_INVALID_CONTEXT                (-32)
#define VERR_TIMER_BUSY                     (-33)
#define VERR_ADDRESS_CONFLICT               (-34)
#define VERR_UNRESOLVED_ERROR               (-35)
#define VERR_INVALID_FUNCTION               (-36)
#define VERR_NOT_SUPPORTED                  (-37)
#define VERR_ACCESS_DENIED                  (-38)
#define VERR_INTERRUPTED                    (-39)
#define VERR_TIMEOUT                        (-40)
#define VERR_BUFFER_OVERFLOW                (-41)
#define VERR_TOO_MUCH_DATA                  (-42)
#define VERR_MAX_THRDS_REACHED              (-43)
#define VERR_MAX_PROCS_REACHED              (-44)
#define VERR_SIGNAL_REFUSED                 (-45)
#define VERR_SIGNAL_PENDING                 (-46)
#define VERR_SIGNAL_INVALID                 (-47)
#define VERR_STATE_CHANGED                  (-48)
#define VERR_INVALID_UUID_FORMAT            (-49)
#define VERR_PROCESS_NOT_FOUND              (-50)
#define VERR_PROCESS_RUNNING                (-51)
#define VERR_TRY_AGAIN                      (-52)
#define VERR_PARSE_ERROR                    (-53)
#define VERR_OUT_OF_RANGE                   (-54)
#define VERR_NUMBER_TOO_BIG                 (-55)
#define VERR_NO_DIGITS                      (-56)
#define VERR_NEGATIVE_UNSIGNED              (-57)
#define VERR_NO_TRANSLATION                 (-58)

#define VERR_NOT_FOUND                      (-78)
#define VERR_INVALID_STATE                  (-79)
#define VERR_OUT_OF_RESOURCES               (-80)

#define VERR_FILE_NOT_FOUND                 (-102)
#define VERR_PATH_NOT_FOUND                 (-103)
#define VERR_INVALID_NAME                   (-104)
#define VERR_ALREADY_EXISTS                 (-105)
#define VERR_TOO_MANY_OPEN_FILES            (-106)
#define VERR_SEEK                           (-107)
#define VERR_NEGATIVE_SEEK                  (-108)
#define VERR_SEEK_ON_DEVICE                 (-109)
#define VERR_EOF                            (-110)
#define VERR_READ_ERROR                     (-111)
#define VERR_WRITE_ERROR                    (-112)
#define VERR_WRITE_PROTECT                  (-113)
#define VERR_SHARING_VIOLATION              (-114)
#define VERR_FILE_LOCK_FAILED               (-115)
#define VERR_FILE_LOCK_VIOLATION            (-116)
#define VERR_CANT_CREATE                    (-117)
#define VERR_CANT_DELETE_DIRECTORY          (-118)
#define VERR_NOT_SAME_DEVICE                (-119)
#define VERR_FILENAME_TOO_LONG              (-120)
#define VERR_MEDIA_NOT_PRESENT              (-121)
#define VERR_MEDIA_NOT_RECOGNIZED           (-122)
#define VERR_FILE_NOT_LOCKED                (-123)
#define VERR_FILE_LOCK_LOST                 (-124)
#define VERR_DIR_NOT_EMPTY                  (-125)
#define VERR_NOT_A_DIRECTORY                (-126)
#define VERR_IS_A_DIRECTORY                 (-127)
#define VERR_FILE_TOO_BIG                   (-128)

#define VERR_NET_IO_ERROR                       (-400)
#define VERR_NET_OUT_OF_RESOURCES               (-401)
#define VERR_NET_HOST_NOT_FOUND                 (-402)
#define VERR_NET_PATH_NOT_FOUND                 (-403)
#define VERR_NET_PRINT_ERROR                    (-404)
#define VERR_NET_NO_NETWORK                     (-405)
#define VERR_NET_NOT_UNIQUE_NAME                (-406)

#define VERR_NET_IN_PROGRESS                    (-436)
#define VERR_NET_ALREADY_IN_PROGRESS            (-437)
#define VERR_NET_NOT_SOCKET                     (-438)
#define VERR_NET_DEST_ADDRESS_REQUIRED          (-439)
#define VERR_NET_MSG_SIZE                       (-440)
#define VERR_NET_PROTOCOL_TYPE                  (-441)
#define VERR_NET_PROTOCOL_NOT_AVAILABLE         (-442)
#define VERR_NET_PROTOCOL_NOT_SUPPORTED         (-443)
#define VERR_NET_SOCKET_TYPE_NOT_SUPPORTED      (-444)
#define VERR_NET_OPERATION_NOT_SUPPORTED        (-445)
#define VERR_NET_PROTOCOL_FAMILY_NOT_SUPPORTED  (-446)
#define VERR_NET_ADDRESS_FAMILY_NOT_SUPPORTED   (-447)
#define VERR_NET_ADDRESS_IN_USE                 (-448)
#define VERR_NET_ADDRESS_NOT_AVAILABLE          (-449)
#define VERR_NET_DOWN                           (-450)
#define VERR_NET_UNREACHABLE                    (-451)
#define VERR_NET_CONNECTION_RESET               (-452)
#define VERR_NET_CONNECTION_ABORTED             (-453)
#define VERR_NET_CONNECTION_RESET_BY_PEER       (-454)
#define VERR_NET_NO_BUFFER_SPACE                (-455)
#define VERR_NET_ALREADY_CONNECTED              (-456)
#define VERR_NET_NOT_CONNECTED                  (-457)
#define VERR_NET_SHUTDOWN                       (-458)
#define VERR_NET_TOO_MANY_REFERENCES            (-459)
#define VERR_NET_CONNECTION_TIMED_OUT           (-460)
#define VERR_NET_CONNECTION_REFUSED             (-461)
#define VERR_NET_HOST_DOWN                      (-464)
#define VERR_NET_HOST_UNREACHABLE               (-465)
#define VERR_NET_PROTOCOL_ERROR                 (-466)
#define VERR_NET_INCOMPLETE_TX_PACKET           (-467)

/* misc. unsorted codes */
#define VERR_RESOURCE_BUSY                      (-138)
#define VERR_DISK_FULL                          (-152)
#define VERR_TOO_MANY_SYMLINKS                  (-156)
#define VERR_NO_MORE_FILES                      (-201)
#define VERR_INTERNAL_ERROR                     (-225)
#define VERR_INTERNAL_ERROR_2                   (-226)
#define VERR_INTERNAL_ERROR_3                   (-227)
#define VERR_INTERNAL_ERROR_4                   (-228)
#define VERR_DEV_IO_ERROR                       (-250)
#define VERR_IO_BAD_LENGTH                      (-255)
#define VERR_BROKEN_PIPE                        (-301)
#define VERR_NO_DATA                            (-304)
#define VERR_SEM_DESTROYED                      (-363)
#define VERR_DEADLOCK                           (-365)
#define VERR_BAD_EXE_FORMAT                     (-608)
#define VINF_HGCM_ASYNC_EXECUTE                 (2903)

#define RT_SUCCESS(rc)      ((rc) >= 0)
#define RT_FAILURE(rc)      ((rc) < 0)

#endif
