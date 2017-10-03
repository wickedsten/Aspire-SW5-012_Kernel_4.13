#ifndef BLK_MQ_SCHED_H
#define BLK_MQ_SCHED_H

#include "blk-mq.h"
#include "blk-mq-tag.h"

void blk_mq_sched_free_hctx_data(struct request_queue *q,
				 void (*exit)(struct blk_mq_hw_ctx *));

void blk_mq_sched_assign_ioc(struct request *rq, struct bio *bio);

void blk_mq_sched_request_inserted(struct request *rq);
bool blk_mq_sched_try_merge(struct request_queue *q, struct bio *bio,
				struct request **merged_request);
bool __blk_mq_sched_bio_merge(struct request_queue *q, struct bio *bio);
bool blk_mq_sched_try_insert_merge(struct request_queue *q, struct request *rq);
void blk_mq_sched_restart(struct blk_mq_hw_ctx *hctx);

void blk_mq_sched_insert_request(struct request *rq, bool at_head,
				 bool run_queue, bool async, bool can_block);
void blk_mq_sched_insert_requests(struct request_queue *q,
				  struct blk_mq_ctx *ctx,
				  struct list_head *list, bool run_queue_async);

void blk_mq_sched_dispatch_requests(struct blk_mq_hw_ctx *hctx);

int blk_mq_init_sched(struct request_queue *q, struct elevator_type *e);
void blk_mq_exit_sched(struct request_queue *q, struct elevator_queue *e);

int blk_mq_sched_init_hctx(struct request_queue *q, struct blk_mq_hw_ctx *hctx,
			   unsigned int hctx_idx);
void blk_mq_sched_exit_hctx(struct request_queue *q, struct blk_mq_hw_ctx *hctx,
			    unsigned int hctx_idx);

int blk_mq_sched_init(struct request_queue *q);

static inline bool
blk_mq_sched_bio_merge(struct request_queue *q, struct bio *bio)
{
	if (blk_queue_nomerges(q) || !bio_mergeable(bio))
		return false;

	return __blk_mq_sched_bio_merge(q, bio);
}

static inline bool
blk_mq_sched_allow_merge(struct request_queue *q, struct request *rq,
			 struct bio *bio)
{
	struct elevator_queue *e = q->elevator;

	if (e && e->type->ops.mq.allow_merge)
		return e->type->ops.mq.allow_merge(q, rq, bio);

	return true;
}

static inline void blk_mq_sched_completed_request(struct request *rq)
{
	struct elevator_queue *e = rq->q->elevator;

	if (e && e->type->ops.mq.completed_request)
		e->type->ops.mq.completed_request(rq);
}

static inline void blk_mq_sched_started_request(struct request *rq)
{
	struct request_queue *q = rq->q;
	struct elevator_queue *e = q->elevator;

	if (e && e->type->ops.mq.started_request)
		e->type->ops.mq.started_request(rq);
}

static inline void blk_mq_sched_requeue_request(struct request *rq)
{
	struct request_queue *q = rq->q;
	struct elevator_queue *e = q->elevator;

	if (e && e->type->ops.mq.requeue_request)
		e->type->ops.mq.requeue_request(rq);
}

static inline bool blk_mq_sched_has_work(struct blk_mq_hw_ctx *hctx)
{
	struct elevator_queue *e = hctx->queue->elevator;

	if (e && e->type->ops.mq.has_work)
		return e->type->ops.mq.has_work(hctx);

	return false;
}

static inline bool blk_mq_sched_needs_restart(struct blk_mq_hw_ctx *hctx)
{
	return test_bit(BLK_MQ_S_SCHED_RESTART, &hctx->state);
}

static inline unsigned blk_mq_sched_queue_depth(struct request_queue *q)
{
	/*
	 * q->queue_depth is more close to scheduler queue, so use it
	 * as hint for computing scheduler queue depth if it is valid
	 */
	unsigned q_depth = q->queue_depth ?: q->tag_set->queue_depth;

	/*
	 * Default to double of smaller one between queue depth and 128,
	 * since we don't split into sync/async like the old code did.
	 * Additionally, this is a per-hw queue depth.
	 */
	q_depth = 2 * min_t(unsigned int, q_depth, BLKDEV_MAX_RQ);

	/*
	 * when queue depth of driver is too small, we set queue depth
	 * of scheduler queue as 128 which is the default setting of
	 * block legacy code.
	 */
	return max_t(unsigned, q_depth, BLKDEV_MAX_RQ);
}

#endif
