#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "seed.h"
#include "template.h"
#include "mail.h"
#include "debug.h"

/* template entry */
extern int SP_init(void);
extern int SINAFX_init(void);

/* Common structure */
struct list_head temp_list;

template_t *get_template_by_name(const char *name)
{
   struct list_head *next;
   template_t *temp;
   int  len,len1;

   len = strlen(name);
   list_for_each(next, &temp_list) {
        temp = get_entry(template_t, list, next);
        len1 = strlen(temp->name);
        if (len != len1)
            continue;
        
        if ( !memcmp(name,temp->name,len)) {
            debug(8,"Find template %s\n",temp->name);
            return temp;
        }
   }

   return NULL; 
}

void parse(const char *buf, int len, void *s)
{
    template_t *temp;
    seed_t *seed = (seed_t *)s;
    
    if (!seed->temp) {
        if ( !(seed->temp = get_template_by_name(seed->template))) {
            printf("Cannot find template %s\n",seed->template);
            return;
        }
    }

    temp = seed->temp;

    temp->notifier(temp->filter(temp->parser(buf,len,seed),seed));

    return;
}

int register_template(template_t *temp)
{
    debug(8,"register template %s\n",temp->name);
    list_add_tail(&temp->list,&temp_list);
    return 0;
}

void tp_send_mail(char *body)
{
    char to[64] = "dchang@juniper.net,callmefire@139.com";
    char from[64] = "\"Eagle\"<eagle@callmefire.com>";
    char draftname[64];
    char domain[16] = "jnpr";
    char subject[32] = "SP News";
    mail_param_t mp;

    sprintf(draftname,"%s/Mail/%ld",getenv("HOME"),random() % 65536);

    memset(&mp,0,sizeof(mp));

    mp.to = to;
    mp.from = from;
    mp.domain = domain;
    mp.subject = subject;
    mp.draftname = draftname;
    mp.body = body;

    if (send_mail(&mp)) {
        perror("Send mail failed");
        return;
    }

    return;
}

void template_init(void)
{
    /* common init */
    INIT_LIST_HEAD(&temp_list);
   
    /* templates init */ 
    SP_init();
    SINAFX_init();
}
