#include <sys/mman.h>

#include "eagle.h"
#include "seed.h"
#include "parser.h"
#include "debug.h"

eagle_t *eagles;
int eagle_num = 1;
char *output_dir = NULL;

unsigned long eagle_ticks = 0;
unsigned long start_tick = 0;

#define EAGLE_USER_AGENT  "Mozilla/5.0 (Windows NT 5.1; rv:8.0) Gecko/20100101 Firefox/8.0"

#define EAGLE_EYE_INVAL   10

/*
 * Usage:
 *
 * iotest 
 *
 * [ -s block_size]                  : IO block size
 * [ -m mode ]                       : IO mode
 * 					1 - seq,sync,non-direct
 * 					2 - seq,async,non-direct
 * 					3 - seq,sync,direct
 * 					4 - seq,async,direct
 * 					5 - ran,sync,non-direct
 * 					6 - ran,async,non-direct
 * 					7 - ran,sync,direct
 * 					8 - ran,async,direct
 * [ -o read/write ]                 : IO operation
 * [ -f file ]                       : target file or device
 * [ -c io_count]                    : io count
 * [ -n num ]                        : subsequent IO thread num
 * [ -t second ]                     : IO time, second
 * [ -l logfile]                     : log file
 * [ -g debug_level                  : enable debug mode and set debug level, default is 0
 */

static void usage(int version)
{
	return;
}

static void timer(int signo)
{
    seed_t *seed;

    debug(1,"timer start working\n");

    while ( (seed = seed_dequeue((seed_q_t *)&seed_ring)) ) {
        seed_enqueue_sem((seed_q_t *)&seed_q,seed,&seed_q.sem);
    }

    eagle_ticks++;
	alarm(EAGLE_EYE_INVAL);
    return;
}

static void eagle_cache_init(int id)
{
    char name[64];

    sprintf(name,"%s/header%d",output_dir,id);

    if ( !(eagles[id].hfp = fopen(name,"w+"))) {
        perror("create header file failed");
        exit(1);
    }    
    
    sprintf(name,"%s/body%d",output_dir,id);
    
    if ( !(eagles[id].bfp = fopen(name,"w+"))) {
        perror("create header file failed");
        exit(1);
    }    
} 

static void eagle_cache_fini(int id)
{
    if (eagles[id].hfp)
        fclose(eagles[id].hfp);

    if (eagles[id].bfp)
        fclose(eagles[id].bfp);
}

static size_t eagle_write_data(void *ptr, size_t size, size_t nmemb, void *fp)
{
  int written = fwrite(ptr, size, nmemb, (FILE *)fp);
  return written;
}

static int get_file_size(int fd)
{
    struct stat fs;
    fstat(fd,&fs);

    return fs.st_size;
}

static char *eagle_cache_map(eagle_t *eagle, int *len)
{
    int fd = fileno(eagle->bfp);
    char *buf;

    *len = get_file_size(fd);
    buf = mmap(NULL,*len, PROT_READ,MAP_PRIVATE,fd,0);

    return (buf == MAP_FAILED)? NULL: buf;
}

int progress_callback(void *clientp,double dltotal,double dlnow,double ultotal,double ulnow)
{
    if ( (eagle_ticks - start_tick) > 5) {
        debug(1,"downloading stopped at %f bytes. reset it\n",dlnow);
        return 1;
    }

    return 0;
}

int watcher(int id)
{
    CURL *curl_handle;
    seed_t *seed;
    eagle_t *eagle = &eagles[id];
    char *buf = NULL;
    int len = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
        
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, progress_callback);
    
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, eagle_write_data);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, EAGLE_USER_AGENT);

    
    for (;;) {
 
        eagle_cache_init(id); 
        
        /* Poll seed queue: FIFO */
        seed = seed_dequeue_sem((seed_q_t *)&seed_q, &seed_q.sem);
        curl_easy_setopt(curl_handle, CURLOPT_URL, seed->url);
        debug(1,"[%d]: poll seed %s\n",id,seed->url);
        
        curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, eagle->hfp);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,   eagle->bfp);

        start_tick = eagle_ticks;
        if (curl_easy_perform(curl_handle)) {
            debug(1,"get url failed (%s)\n",seed->url);
        } else {
        
            buf = eagle_cache_map(eagle, &len);
            if (!buf) {
                perror("mmap failed");
            } else {
                debug(1,"cache[%d]: size %d\n",id,len);
                parser(buf, len, seed);
           
                munmap(buf,len); 
            }
        } 
        
        seed_enqueue((seed_q_t *)&seed_ring,seed);
        eagle_cache_fini(id);
        debug(1,"[%d]: done\n",id);
    }

	return 0;
}

void eagles_init(void)
{
    eagles = (eagle_t *)calloc(eagle_num,sizeof(eagle_t));
    if (!eagles) {
        perror("eagles allocation failed");
        exit(1);
    }
}

int main(int argc,char **argv)
{
	signed char c;
	struct option long_options[] = {
		{"seed"   , 1, 0, 's'},
		{"mode"   , 1, 0, 'm'},
		{"output-dir" , 1, 0, 'o'},
		{"file"   , 1, 0, 'f'},
		{"count"  , 1, 0, 'c'},
		{"num"    , 1, 0, 'n'},
		{"time"   , 1, 0, 't'},
		{"log"    , 1, 0, 'l'},
		{"help"   , 0, 0, 'h'},
		{"version", 0, 0, 'v'},
		{0, 0, 0, 0}
	};
	int option_index = 0; 
	int ret = 0;
	int i;
    char *seedname = NULL;

	while ( (c = getopt_long(argc,argv,"b:m:o:f:c:s:d:n:t:l:g:hv",long_options,&option_index)) 
		> 0) {
		
		switch (c) {
			case 'b':
				break;
			case 'm':
				break;
			case 'o':
                if (!optarg) {
                    perror("No output dir specified");
                    exit(1);
                }
                output_dir = optarg;
				break;
			case 'f':
				break;
			case 'c':
				break;
			case 'n':
                if ( (eagle_num = atoi(optarg)) == 0) {
                    perror("Wrong eagle number");
                    exit(1);
                }
				break;
            case 's':
                if (!optarg) {
                    perror("No seeds specified");
                    exit(1);
                }
                seedname = optarg;
                break;
			case 't':
				break;
			case 'l':
			case 'g':
				break;
			case 'v':
				usage(1);
				exit(0);
			case 'h':
			case ':':
			case '?':
			default:
				usage(0);
				exit(0);
		}
	}

    eagles_init();
    seeds_init();
    seeds_file_init(seedname);
    parser_init();
	
    /* setup signal handler */
	if (signal(SIGALRM,timer) == SIG_ERR) {
		perror("signal setup failed");
		goto err;
	}
	

	alarm(EAGLE_EYE_INVAL);
			
	/* create eagles */
	for (i=0; i < eagle_num; i++) {
		if ( pthread_create(&eagles[i].pid,NULL,(void *)watcher,(void *)i) != 0) {
			printf("eagle %d failed\n",i);
			goto err;
		}
	}

#if 1
	while (1)
		pause();
#else	
	for (i=0; i < eagle_num; i++)
		pthread_join(eagles[i].pid,NULL);
#endif	
err:	

	return ret;
}


