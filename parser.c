#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "template.h"
#include "debug.h"

struct list_head temp_list;

static template_t *get_template_by_name(const char *name)
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
            debug(1,"Find template %s\n",temp->name);
            return temp;
        }
   }

   return NULL; 
}

void parser(const char *buf, int len, seed_t *seed)
{
    template_t *temp;
    
    if (!seed->temp) {
        if ( !(seed->temp = get_template_by_name(seed->template))) {
            printf("Cannot find template %s\n",seed->template);
            return;
        }
    }

    temp = seed->temp;

    temp->notifier(temp->filter(temp->parser(buf,len,temp),temp));

    return;
}

int register_template(template_t *temp)
{
    list_add_tail(&temp->list,&temp_list);
    return 0;
}

void parser_init(void)
{
    INIT_LIST_HEAD(&temp_list);

    template_init();
}
