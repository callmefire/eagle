#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include "seed.h"
#include "template.h"
#include "debug.h"

static template_t *template = NULL;

typedef struct entry {
    char name[8];
    char time[10];
    char price[8];
#define TO_BE_REPORTED 0x1
    int flag;
} entry_t;

static int get_entry_num(const char *buf, int len)
{
    regex_t preg;
    regmatch_t pmatch;
    char *regex = "\\bvar\\b";
    int cnt = 0;
    char *p = (char *)buf;

    if (regcomp(&preg,regex,0)) {
         perror("Regex compile error\n");
         return 1;
    }

    while ( regexec(&preg,p,1,&pmatch, 0) != REG_NOMATCH) {
        cnt++;
        p = &p[pmatch.rm_eo];
    }
    regfree(&preg);

    return cnt;
}

static int get_entry_name(const char *buf, TP_header_t *hdr)
{
    regex_t preg;
    regmatch_t pmatch;
    char *regex = "[A-Z]\\{6,6\\}";
    char *p = (char *)buf;
    entry_t *ep = (entry_t *)hdr->data;
    int i = 0, len;

    if (regcomp(&preg,regex,0)) {
         perror("Regex compile error\n");
         return 1;
    }
    
    while ( regexec(&preg,p,1,&pmatch, 0) != REG_NOMATCH) {
        len = pmatch.rm_eo - pmatch.rm_so;
        memcpy(&ep[i].name, &p[pmatch.rm_so],len);
        ep[i].name[len] = 0;

        debug(6,"get name %s\n",ep[i].name);
        
        i++;
        if (i >= hdr->number)
            break;
        
        p = &p[pmatch.rm_eo];
    }
    
    regfree(&preg);

    return 0;
}

static int get_entry_time(const char *buf, TP_header_t *hdr)
{
    regex_t preg;
    regmatch_t pmatch;
    char *regex = "\\([0-9]\\{2,2\\}:\\)\\{2,2\\}[0-9]\\{2,2\\}";
    char *p = (char *)buf;
    entry_t *ep = (entry_t *)hdr->data;
    int i = 0, len;

    if (regcomp(&preg,regex,0)) {
         perror("Regex compile error\n");
         return 1;
    }
    
    while ( regexec(&preg,p,1,&pmatch, 0) != REG_NOMATCH) {
        len = pmatch.rm_eo - pmatch.rm_so;
        memcpy(&ep[i].time, &p[pmatch.rm_so], len);
        ep[i].time[len] = 0;

        debug(6,"get time %s\n",ep[i].time);
        
        i++;
        if (i >= hdr->number)
            break;
        
        p = &p[pmatch.rm_eo];
    }

    regfree(&preg);
    return 0;
}

static int get_entry_price(const char *buf, TP_header_t *hdr)
{
    regex_t preg1;
    regex_t preg2;
    regmatch_t pmatch;
    char *regex1 = "\\bvar\\b";
    char *regex2 = "[0-9]\\+\\?\\.[0-9]\\+\\?";
    char *p = (char *)buf;
    entry_t *ep = (entry_t *)hdr->data;
    int i = 0, len;

    if (regcomp(&preg1,regex1,0)) {
         perror("Regex compile error\n");
         return 1;
    }
    
    if (regcomp(&preg2,regex2,0)) {
         perror("Regex compile error\n");
         regfree(&preg1);
         return 1;
    }
    
    regexec(&preg1,p,1,&pmatch, 0);
    do {
        p = &p[pmatch.rm_eo];

        if (regexec(&preg2,p,1,&pmatch,0) != REG_NOMATCH) {
            len = pmatch.rm_eo - pmatch.rm_so;
            memcpy(&ep[i].price, &p[pmatch.rm_so],len);
            ep[i].price[len] = 0;
        }
        debug(6,"get price %s\n",ep[i].price);
        
        i++;
        if (i >= hdr->number)
            break;
        
    } while ( regexec(&preg1,p,1,&pmatch, 0) != REG_NOMATCH);
    
    regfree(&preg1);
    regfree(&preg2);

    return 0;
}

static void *parser(const char *buf, int len, void *seed)
{  
    char *p;
    char *end;
    int num;
    TP_header_t *hdr = NULL;
   
    if (!buf || !seed)
        goto error;

    p = (char *)buf;
    end = (char *)buf + len;

    num = get_entry_num(buf,len);
    debug(8,"get entry %d, need size %d\n",num,sizeof(TP_header_t)+num*sizeof(entry_t));

    hdr = (TP_header_t *)malloc(sizeof(TP_header_t)+ num * sizeof(entry_t));
    if (!hdr) {
        perror("No memory");
        goto error;
    }

    hdr->number = num;

    if (get_entry_name(buf,hdr))
        goto error;
    if (get_entry_time(buf,hdr))
        goto error;
    if (get_entry_price(buf,hdr))
        goto error;

    return (void *)hdr;

error:
    if (hdr)
        free(hdr);
    return NULL;
}

static void *filter(void *data, void *s) 
{
    seed_t *seed = s;
    TP_header_t *old;
    TP_header_t *new;
    entry_t *np;
    entry_t *op;
    double nv,ov;
    int i, has_new = 0;
    
    if (!data)
        return NULL;
        
    if (!seed->private) {
        seed->private = data;
        return NULL;
    }

    old = (TP_header_t *)seed->private;
    new = (TP_header_t *)data;

    if (old->number != new->number)
        return NULL;

    np = (entry_t *)new->data;
    op = (entry_t *)old->data;

    for (i=0; i< new->number; i++) {
        nv = atof(np[i].price);
        ov = atof(op[i].price);
        
        if (i == 0) {
           if ( ((nv - ov) > 0.2) || ((ov - nv) > 0.2)) {
                np[i].flag |= TO_BE_REPORTED;
                has_new = 1;
           } else {
                np[i] = op[i];
           }
        } else if ((i == 3) || (i == 5) || (i == 7)) {
           if ( ((nv - ov) > 0.002) || ((ov - nv) > 0.002)) {
                np[i].flag |= TO_BE_REPORTED;
                has_new = 1;
           } else {
                np[i] = op[i];
           }
        } else {
            np[i].flag = 0;
        }
    }

    free(seed->private);
    seed->private = data;
   
    if (!has_new)
        data = NULL;

    return data;
}

static void notifier(void *data, void * s)
{
    seed_t *seed = (seed_t *)s;
    TP_header_t *hdr;
    entry_t *ep;
    int i, num = 0;
    char *buf;
    char *p;
    
    debug(8,"enter %s: data %p\n",__FUNCTION__,data);
    
    if (!data)
        return;

    hdr = (TP_header_t *)data;
    ep = (entry_t *)hdr->data;

    buf = (char *)calloc(1, hdr->number * (sizeof(entry_t) + 32));
    if (!buf) {
        perror("No mem to alloc buf for notifier");
        return;
    }

    p = buf;

    for (i=0; i<hdr->number; i++) {
        if (!(ep[i].flag & TO_BE_REPORTED)) 
            continue;
        debug(6, "Find new data\n"); 
        num = sprintf(p,"[%d]: %s, %s, %s\n",i,ep[i].name,ep[i].time,ep[i].price);
        p += num;
        ep[i].flag &= ~TO_BE_REPORTED;
    }

    *p = 0;

    if (p != buf && seed->mail)
        tp_send_mail(seed->mail, "jnpr", "SINAFX", buf); 
   
    free(buf);
    return;
}

int SINAFX_init(void)
{
    template = calloc(1,sizeof(template_t));

    if (!template) {
        perror("Alloc SP template failed");
        return -1;
    }
    
    INIT_LIST_HEAD(&template->list);

    template->name[0] = 'S';
    template->name[1] = 'I';
    template->name[2] = 'N';
    template->name[3] = 'A';
    template->name[4] = 'F';
    template->name[5] = 'X';
    template->parser = parser;
    template->filter = filter;
    template->notifier = notifier;

    register_template(template);
    return 0;
}
