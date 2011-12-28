#include "tako.h"
#include "seed.h"
#include "debug.h"

tako_t *takos;
int tako_num = 1;
char *output_dir = NULL;

#define TAKO_USER_AGENT  "Mozilla/5.0 (Windows NT 5.1; rv:8.0) Gecko/20100101 Firefox/8.0"

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

static void time_handler(int signo)
{

}

static void tako_cache_init(int id)
{
    char name[64];

    sprintf(name,"%s/theader%d",output_dir,id);

    if ( !(takos[id].hfp = fopen(name,"w+"))) {
        perror("create header file failed");
        exit(1);
    }    
    
    sprintf(name,"%s/tbody%d",output_dir,id);
    
    if ( !(takos[id].bfp = fopen(name,"w+"))) {
        perror("create header file failed");
        exit(1);
    }    
} 

static void tako_cache_fini(int id)
{
    if (takos[id].hfp)
        fclose(takos[id].hfp);

    if (takos[id].bfp)
        fclose(takos[id].bfp);
}

static size_t tako_write_data(void *ptr, size_t size, size_t nmemb, void *fp)
{
  int written = fwrite(ptr, size, nmemb, (FILE *)fp);
  return written;
}

int creep(int id)
{
    CURL *curl_handle;
    seed_t *seed;
    unsigned int hash;
    tako_t *tako = &takos[id];

    tako_cache_init(id); 

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
        
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, tako_write_data);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, TAKO_USER_AGENT);

    //mmap();
    
    for (;;) {
 
        //poll seed queue
        seed = seed_dequeue_sem((seed_q_t *)&seed_q, &seed_q.sem);
        curl_easy_setopt(curl_handle, CURLOPT_URL, seed->url);
        debug(1,"[%d]: poll seed %s\n",id,seed->url);
        
        curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, tako->hfp);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA,   tako->bfp);

        if (curl_easy_perform(curl_handle)) {
            debug(1,"get url failed (%s)\n",seed->url);
            seed_free(seed);
            continue;
        }

        hash = seed_hash(seed->url);
        seed_enqueue((seed_q_t *)&seed_base[hash],seed);

        debug(1,"[%d]: done\n",id);
        //paser
    }

    tako_cache_fini(id);

	return 0;
}

void takos_init(void)
{
    takos = (tako_t *)calloc(tako_num,sizeof(tako_t));
    if (!takos) {
        perror("takos allocation failed");
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
                if ( (tako_num = atoi(optarg)) == 0) {
                    perror("Wrong tako number");
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

    takos_init();
    seeds_init(65535);
    seeds_file_init(seedname);
	
    /* setup signal handler */
	if (signal(SIGALRM,time_handler) == SIG_ERR) {
		perror("signal setup failed");
		goto err;
	}
	

	//alarm(1);
			
	/* create takos */
	for (i=0; i < tako_num; i++) {
		if ( pthread_create(&takos[i].pid,NULL,(void *)creep,(void *)i) != 0) {
			printf("tako %d failed\n",i);
			goto err;
		}
	}

#if 1
	while (1)
		pause();
#else	
	for (i=0; i < tako_num; i++)
		pthread_join(takos[i].pid,NULL);
#endif	
err:	

	return ret;
}


