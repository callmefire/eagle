#ifndef __SEED_H__
#define __SEED_H__

#include <semaphore.h>
#include "list.h"
#include "template.h"

typedef struct seed {
    struct list_head list;
    char *url;
    int flags;
    int interval;
    int time;
    char *template;
    template_t *temp;
    void *private;
} seed_t;

typedef struct seed_q {
    struct list_head head;
    pthread_mutex_t q_mutex;
}   seed_q_t;

typedef struct seed_j_q {
    struct list_head head;
    pthread_mutex_t q_mutex;
    sem_t sem;
} seed_job_q;

typedef struct seed_h_q {
    struct list_head head;
    pthread_mutex_t q_mutex;
} seed_hash_q;

typedef struct seed_r_q {
    struct list_head head;
    pthread_mutex_t q_mutex;
} seed_ring_q;

extern seed_job_q seed_q;
extern seed_hash_q *seed_base;
extern seed_ring_q seed_ring;

extern void queue_init(seed_q_t *q);
extern void seeds_init(unsigned int);
extern void seeds_cfg_init(const char *);

extern void seed_enqueue(seed_q_t *,seed_t *);
extern seed_t *seed_dequeue(seed_q_t *);
extern seed_t *seed_try_dequeue(seed_q_t *queue);
extern void seed_queue_move(seed_q_t *from, seed_q_t *to);

extern void seed_enqueue_sem(seed_q_t *,seed_t *,sem_t *sem);
extern seed_t *seed_dequeue_sem(seed_q_t *, sem_t *sem);

extern seed_t *seed_alloc(void);
extern void seed_free(seed_t *);
extern unsigned int seed_hash(const char *);
#endif
