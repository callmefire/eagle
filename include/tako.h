#ifndef __TAKO_H__
#define __TAKO_H__

/* Glibc headers */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <sys/stat.h>

#include <time.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>

/* Kernel headers */
#include <asm/ioctl.h>
#include <asm/fcntl.h>

/* Other lib headers */
#include <curl/curl.h>
#include <pthread.h>

/* Private headers */
#include "list.h"

#define MAX(a,b)    ((a)>(b))?(a):(b)
#define MIN(a,b)    ((a)>(b))?(b):(a)

typedef struct tako_ {
	pthread_t pid;
    FILE *hfp;
    FILE *bfp;
    int flags;
    unsigned int total_cnt;         /* creeped url counter */
    unsigned int total_bytes;
} tako_t;


extern tako_t *takos;
extern int tako_num;

#endif
