#include "tako.h"
#include "seed.h"
#include "hash.h"
#include "debug.h"

seed_job_q seed_q;
seed_hash_q *seed_base;
seed_ring_q seed_ring;
        
unsigned int seed_hash(const char *str)
{
    return strhash(str) % SEED_HASH;
}

seed_t *seed_match(const char *url)
{
    unsigned int hash;
    int len;
    struct list_head *next;
    seed_t *s;
    seed_q_t *q;

    if (!url) 
        return NULL; 

    hash = seed_hash(url);
    q = (seed_q_t *)&seed_base[hash];

    pthread_mutex_lock(&q->q_mutex);
    list_for_each(next,&q->head) {
        s = get_entry(seed_t,list,next);
        
        len = MIN(strlen(url),strlen(s->url)) + 1;
        if (!memcmp(url,s->url,len)) {
            pthread_mutex_unlock(&q->q_mutex);
            return s;
        }
    }
    pthread_mutex_unlock(&q->q_mutex);

    return NULL;
}

void seed_enqueue(seed_q_t *queue, seed_t *seed)
{
    pthread_mutex_lock(&queue->q_mutex);
    list_add_tail(&seed->list, &queue->head);     
    pthread_mutex_unlock(&queue->q_mutex);
    debug(1,"enqueue %s\n",seed->url);
}

seed_t *seed_dequeue(seed_q_t *queue)
{
    seed_t *s;
    
    pthread_mutex_lock(&queue->q_mutex);
    s = list_empty(&queue->head)?NULL:get_entry(seed_t,list,queue->head.next); 
    if (s) {
        list_del_init(&s->list); 
        debug(1,"dequeue %s\n",s->url);
    }
    pthread_mutex_unlock(&queue->q_mutex);

    return s;
}

void seed_enqueue_sem(seed_q_t *queue, seed_t *seed, sem_t *sem)
{
    pthread_mutex_lock(&queue->q_mutex);
    list_add_tail(&seed->list, &queue->head);     
    pthread_mutex_unlock(&queue->q_mutex);
    
    debug(1,"enqueue %s\n",seed->url);
    sem_post(sem);
}

seed_t *seed_dequeue_sem(seed_q_t *queue, sem_t *sem)
{
    seed_t *s;
   
    sem_wait(sem);

    pthread_mutex_lock(&queue->q_mutex);
    s = get_entry(seed_t,list,queue->head.next); 
    list_del_init(&s->list); 
    pthread_mutex_unlock(&queue->q_mutex);
    
    debug(1,"dequeue %s\n",s->url);

    return s;
}

seed_t *seed_alloc(void)
{
   seed_t *s = (seed_t *)calloc(1,sizeof(seed_t));
   if (s) {
        INIT_LIST_HEAD(&s->list);
   }

   return s; 
}

void seed_free(seed_t *seed)
{
    if (!seed)
       return;

    if (seed->url)
        free(seed->url);
    
    if (seed->filter)
        free(seed->filter);
    
    if (seed->template)
        free(seed->template);

    free(seed);
}

#define STM_GET_SEED     0x1
#define STM_GET_URL      0x2
#define STM_GET_FILTER   0x3
#define STM_GET_TEMPLATE 0x4

struct seed_stack_node {
    int state;
    char *pos;
};

struct seed_stack {
    struct seed_stack_node stack[8];
    int top;
};

static void seed_node_push(struct seed_stack *stack, int state, char *pos)
{
    stack->top++;
    stack->stack[stack->top].state = state;   
    stack->stack[stack->top].pos = pos;
}

static struct seed_stack_node *seed_node_pop(struct seed_stack *stack) {
    struct seed_stack_node *ret = &stack->stack[stack->top];
    stack->top--;

    return ret;
}

static void seed_stack_init(struct seed_stack *stack) {
    memset(stack,0,sizeof(struct seed_stack));
    stack->top = -1;
}

#if 0
static int get_cur_state(struct seed_stack *stack) {
    return stack->stack[stack->top].state;
}

static int is_stack_empty(struct seed_stack *stack) {
    return (stack->top < 0)?1:0;
}
#endif

static void seed_parse(FILE *fp)
{
    char buf[2048];
    char *start;
    int line = 1;
    struct seed_stack stack;
    struct seed_stack_node *node = NULL;
    seed_t *seed = NULL;
    seed_stack_init(&stack);

    while (fgets(buf,sizeof(buf),fp)) {
        if (buf[0] == 0) {
            line++;
            continue;
        }
        start = buf;
repeat:
        if (!(start=strstr(start,"<"))) {
            line++;
            continue;
        }
       
        if (*(start+1) != '/') {
            if (!memcmp(start,"<seed>",6)) {
                start += 6;
                seed_node_push(&stack,STM_GET_SEED,start);
                debug(1,"push <seed>\n");
                if (!(seed = seed_alloc())) {
                    perror("Seed alloc failed");
                    exit(1);
                }
            } else if (!memcmp(start,"<url>",5)) {
                start += 5;
                seed_node_push(&stack,STM_GET_URL,start);
                debug(1,"push <url>\n");
            } else if (!memcmp(start,"<filter>",8)) {
                start += 8;
                seed_node_push(&stack,STM_GET_FILTER,start);
                debug(1,"push <filter>\n");
            } else if (!memcmp(start,"<template>",10)) {
                start += 10;
                seed_node_push(&stack,STM_GET_TEMPLATE,start);
                debug(1,"push <template>\n");
            } else {
                start++; 
            }
        } else {
            if (!memcmp(start,"</seed>",7)) {
                node = seed_node_pop(&stack);
                debug(1,"pop <seed>\n");
                seed_enqueue_sem((seed_q_t *)&seed_q,seed,&seed_q.sem);
                seed = NULL;
                start += 7;
            } else if (!memcmp(start,"</url>",6)) {
                int len;
                node = seed_node_pop(&stack);
                debug(1,"pop <url>\n");
                len = start - node->pos;
                seed->url = malloc(len);
                if (!seed->url) {
                    debug(1,"URL alloc failed: %d\n",len);
                    exit(1);
                }
                memcpy(seed->url,node->pos,len);
                start += 6;
            } else if (!memcmp(start,"</filter>",9)) {
                int len;
                node = seed_node_pop(&stack);
                debug(1,"pop <filter>\n");
                len = start - node->pos;
                seed->filter = malloc(len);
                memcpy(seed->filter,node->pos,len);
                start += 9;
            } else if (!memcmp(start,"</template>",11)) {
                int len;
                node = seed_node_pop(&stack);
                debug(1,"pop <template>\n");
                len = start - node->pos;
                seed->template= malloc(len);
                memcpy(seed->template,node->pos,len);
                start += 11;
            } else {
                start += 2;
            }

        }

        goto repeat; 
    }    
}

static void queue_init(seed_q_t *q)
{
    INIT_LIST_HEAD(&q->head);
	pthread_mutex_init(&q->q_mutex,NULL);
}

void seeds_init(void)
{ 
    int i;

    /* Init job queue */
    queue_init((seed_q_t *)&seed_q);   
    sem_init(&seed_q.sem,0,0);

    /* Init hash */
    seed_base = calloc(SEED_HASH,sizeof(seed_hash_q));
    if (!seed_base) {
        perror("seed hash alloc failed\n");
        exit(1);
    }

    for (i=0; i<SEED_HASH; i++) {
        queue_init((seed_q_t *)&seed_base[i]);
    }

    queue_init((seed_q_t *)&seed_ring);
}

void seeds_file_init(const char *name)        
{
    FILE *fp;

    fp = fopen(name,"r");
    if (!fp) {
        perror("Seed file not found");
        return;
    }
    
    seed_parse(fp);
    fclose(fp);
}

