#ifndef __HAHS_H__
#define __HASH_H__

/* BKDR String Hash */
unsigned int strhash(const char *str)
{
    unsigned int seed = 131313; /* 31 131 1313 13131 131313 ... */
    unsigned int hash = 0;

    while (*str) {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

#endif
