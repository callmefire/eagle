#include "tako.h"
#include "seed.h"
#include "hash.h"
#include "debug.h"

#include <sys/mman.h>
#include <regex.h>

seed_job_q seed_q;
seed_hash_q *seed_base;
seed_ring_q seed_ring;
unsigned int seed_hash_size;
        
unsigned int seed_hash(const char *str)
{
    return strhash(str) % seed_hash_size;
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
    debug(8,"enqueue %s\n",seed->url);
}

seed_t *seed_dequeue(seed_q_t *queue)
{
    seed_t *s;
    
    pthread_mutex_lock(&queue->q_mutex);
    s = list_empty(&queue->head)?NULL:get_entry(seed_t,list,queue->head.next); 
    if (s) {
        list_del_init(&s->list); 
        debug(8,"dequeue %s\n",s->url);
    }
    pthread_mutex_unlock(&queue->q_mutex);

    return s;
}

void seed_enqueue_sem(seed_q_t *queue, seed_t *seed, sem_t *sem)
{
    pthread_mutex_lock(&queue->q_mutex);
    list_add_tail(&seed->list, &queue->head);     
    pthread_mutex_unlock(&queue->q_mutex);
    
    debug(8,"enqueue %s\n",seed->url);
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
    
    debug(8,"dequeue %s\n",s->url);

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
    int pos;
};

struct seed_stack {
    struct seed_stack_node stack[8];
    int top;
};

static void seed_node_push(struct seed_stack *stack, int state, int pos)
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

static void seed_parse(const char *buf, unsigned int len)
{
    char *start;
    char *end;
    char *p;
    struct seed_stack stack;
    struct seed_stack_node *node = NULL;
    seed_t *seed = NULL;
    
    regex_t preg;
    regmatch_t pmatch;
    char *regex = "<\\(/\\)\\?\\w\\+\\?>"; /* <xxx> or </xxx> */
    int ret,key_len,diff;

    if (regcomp(&preg,regex,0)) {
        perror("Regex compile error\n");
        exit(1);
    }

    seed_stack_init(&stack);

    start = (char *)buf;
    end = (char *)(buf + len);

    p = start;

    while (p <= end) {

        if ( (ret = regexec(&preg,p,1,&pmatch, 0))) {
            break;
        }
        
        diff = (int)(p - start);
        key_len = pmatch.rm_eo - pmatch.rm_so - 2;  /* <,> */
        p = &p[pmatch.rm_so+1];

        if ( *p != '/') {
            if (!memcmp(p,"seed",key_len)) {
                seed_node_push(&stack,STM_GET_SEED,pmatch.rm_eo + diff);
                debug(8,"push <seed>\n");
                if (!(seed = seed_alloc())) {
                    perror("Seed alloc failed");
                    exit(1);
                }
            } else if (!memcmp(p,"url",key_len)) {
                seed_node_push(&stack,STM_GET_URL,pmatch.rm_eo + diff);
                debug(8,"push <url>\n");
            } else if (!memcmp(p,"template",key_len)) {
                seed_node_push(&stack,STM_GET_TEMPLATE,pmatch.rm_eo + diff);
                debug(8,"push <template>\n");
            } else {
                ; 
            }
        } else {
            p++;
            key_len--;
            
            if (!memcmp(p,"seed",key_len)) {
                node = seed_node_pop(&stack);
                debug(8,"pop <seed>\n");
                seed_enqueue_sem((seed_q_t *)&seed_q,seed,&seed_q.sem);
                seed = NULL;
            } else if (!memcmp(p,"url",key_len)) {
                int len;
                node = seed_node_pop(&stack);
                debug(8,"pop <url>\n");
                len = pmatch.rm_so + diff - node->pos + 1;
                seed->url = calloc(1,len);
                if (!seed->url) {
                    debug(1,"URL alloc failed: %d\n",len);
                    exit(1);
                }
                memcpy(seed->url,&start[node->pos],len - 1);
            } else if (!memcmp(p,"template",key_len)) {
                int len;
                node = seed_node_pop(&stack);
                debug(8,"pop <template>\n");
                len = pmatch.rm_so + diff - node->pos + 1;
                seed->template = calloc(1,len);
                if (!seed->template) {
                    debug(1,"template alloc failed: %d\n",len);
                    exit(1);
                }
                memcpy(seed->template,&start[node->pos],len - 1);
            } else {
                ;
            }
        }
        p = &start[pmatch.rm_eo+diff];
    }
}

static void queue_init(seed_q_t *q)
{
    INIT_LIST_HEAD(&q->head);
	pthread_mutex_init(&q->q_mutex,NULL);
}

static int get_file_size(int fd)
{
    struct stat fs;
    fstat(fd,&fs);

    return fs.st_size;
}

void seeds_init(unsigned int size)
{ 
    int i;

    /* Init job queue */
    queue_init((seed_q_t *)&seed_q);   
    sem_init(&seed_q.sem,0,0);

    seed_hash_size = size;

    /* Init hash */
    seed_base = calloc(seed_hash_size,sizeof(seed_hash_q));
    if (!seed_base) {
        perror("seed hash alloc failed\n");
        exit(1);
    }

    for (i=0; i<seed_hash_size; i++) {
        queue_init((seed_q_t *)&seed_base[i]);
    }

    queue_init((seed_q_t *)&seed_ring);
}

void seeds_file_init(const char *name)        
{
    FILE *fp;
    char *buf;
    unsigned int len;

    fp = fopen(name,"r");
    if (!fp) {
        perror("Seed file not found");
        return;
    }

    len = get_file_size(fileno(fp));
    
    buf = mmap(NULL,len, PROT_READ,MAP_PRIVATE,fileno(fp),0); 
    if (!buf) {
        perror("Seed file map failed");
        return;
    }

    seed_parse(buf,len);
    fclose(fp);
}

