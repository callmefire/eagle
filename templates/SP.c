#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "seed.h"
#include "parser.h"
#include "template.h"
#include "debug.h"

static template_t *SP_temp = NULL;

typedef struct SP_entry {
    char entity[128];
    char date[64];
    char to[16];
    char from[16];
    char action[16];
    char type[16];
#define SP_ENTRY_NEW 0x1
    int flag;
} SP_entry_t;

/*
 * 1. Find "tabContent2"
 * 2. Find "Entity name"
 * 3. Find "<tbody>
 * 4. Find "<tr>
 * 5. Find "<td>" - Entity
 * 6. Find "<td>" - Date
 * 7. Find "<td>" - To
 * 8. Find "<td>" - From
 * 9. Find "<td>" - Action 
 *10. Find "<td>" - Type
 */

#define TABCONTENT2  "tabContent2"
#define ENTITYNAME   "Entity Name"
#define TBODYSTART   "<tbody>"
#define TBODYEND     "</tbody>"

char *get_tbody_start(const char *buf, int len)
{
    char *p;
    
    p = strstr(buf,TABCONTENT2);
    if (!p) {
        debug(1,"Cannot find %s\n",TABCONTENT2);
        return NULL;
    }
    
    p = strstr(p,ENTITYNAME);
    if (!p) {
        debug(1,"Cannot find %s\n",ENTITYNAME);
        return NULL;
    }
    
    p = strstr(p,TBODYSTART);
    if (!p) {
        debug(1,"Cannot find %s\n",TBODYSTART);
        return NULL;
    }
    
    return p;
}

char *get_tbody_end(char *start)
{
    char *p = NULL;

    p = strstr(start,TBODYEND);

    return p;
}

int get_entry_num(char *start, char *end)
{
    char *p = start;
    char *q;
    int ret = 0;
    
    while ( (q = strstr(p,"<tr>")) && (q < end)) {
        p = q + 4;
        ret++;
   }
   
   debug(1,"enter %s: start 0x%p, end 0x%p, num %d\n",__FUNCTION__,start,end,ret);
   
   return (ret - 1);
}

static void extract_data(const char *start, const char *end, char *data)
{
    int i = 0;
    char *p = (char *)start;
    
    debug(1,"enter %s\n",__FUNCTION__);
    
    while (p < end) {
        if ( *p == '<') {
            p = strstr(p,">");
        } else if ( *p == '\r') {
            ;
        } else if ( *p == '\n') {
            ;
        } else if ( *p == '\t') {
            ;
        } else {
            if ( (*p == ' ') && (*(p+1) == ' ')) {
                ;
            } else {
                data[i] = *p;
                i++;
            }
        }
        p++;
    }
    
    data[i] = 0;
    
    debug(1,"extract data: %s\n",data);
    
    return;
}

char *get_sp_entry(char *start, SP_entry_t *entry)
{
    char *tr_start;
    char *tr_end;
    char *td_start;
    char *td_end;

    tr_start = strstr(start,"<tr>");
    tr_end   = strstr(start,"</tr>");
    td_end   = tr_start + 4;

    td_start = strstr(td_end,"<td");
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->entity);
    
    td_start = strstr(td_end,"<td");
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->date);
    
    td_start = strstr(td_end,"<td");
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->to);
     
    td_start = strstr(td_end,"<td");
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->from);
     
    td_start = strstr(td_end,"<td");
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->action);
     
    td_start = strstr(td_end,"<td");
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->type);
    
    return tr_end+5;
}

void *SP_parser(const char *buf, int len, void *tp)
{   
    char *tbody_start;
    char *tbody_end;
    char *p;
    int num = 0, i;
    TP_header_t *hdr = NULL;
    SP_entry_t *entry = NULL;
    
    debug(1,"enter %s\n",__FUNCTION__);
    
    tbody_start = get_tbody_start(buf,len);
    if (!tbody_start) {
        debug(1,"Cannot find tobody start\n");
        return NULL;
    }
    
    tbody_end = get_tbody_end(tbody_start);
    if (!tbody_end) {
        debug(1,"Cannot find tobody end\n");
        return NULL;
    }
    
    num = get_entry_num(tbody_start,tbody_end);
    if (num <= 0) {
        debug(1,"Cannot find entry\n");
        return NULL;
    }
    
    hdr = (TP_header_t *)malloc(sizeof(TP_header_t)+ num * sizeof(SP_entry_t));
    if (!hdr) {
        perror("Cannot alloc SP header and entry\n");
        return NULL;
    }
    
    memset(hdr,0,sizeof(TP_header_t) + num * sizeof(SP_entry_t));
    
    hdr->number = num;
    entry = (SP_entry_t *)hdr->data;
    
    p = tbody_start;
    for (i=0; i<num; i++) {
        p = get_sp_entry(p, &entry[i]);
    }
    
    return hdr;
}

void *SP_filter(void *data, void *tp) {
    template_t *temp = (template_t *)tp;
    TP_header_t *old;
    TP_header_t *new;
    SP_entry_t *np;
    SP_entry_t *op;
    int i,j,match;
    int r1,r2,r3,r4,r5,r6;
    
    if (!data)
        return NULL;
        
    if (!temp->private) {
        temp->private = data;
        return NULL;
    }

    old = (TP_header_t *)temp->private;
    new = (TP_header_t *)data;

    np = (SP_entry_t *)new->data;

    for (i=0; i< new->number; i++) {
        op = (SP_entry_t *)old->data;
        match = 0;
        
        for (j=0; j<old->number; j++) {
#if 1
            if ( !(r1 = memcmp(np->entity, op->entity, strlen(np->entity))) &&
                 !(r2 = memcmp(np->date,   op->date,   strlen(np->date)))   &&
                 !(r3 = memcmp(np->to,     op->to,     strlen(np->to)))     &&
                 !(r4 = memcmp(np->from,   op->from,   strlen(np->from)))   &&
                 !(r5 = memcmp(np->action, op->action, strlen(np->action))) &&
                 !(r6 = memcmp(np->type,   op->type,   strlen(np->type))) ) {
#endif
                match = 1;
                break;
            }

            op++;
        }
        
        if (!match && strstr(np->action,"Revised") && strstr(np->type,"Foreign")) {
            np->flag |= SP_ENTRY_NEW;
            printf("%d,%d,%d,%d,%d,%d\n",r1,r2,r3,r4,r5,r6);
        }
        np++;
    }

    free(temp->private);
    temp->private = data;

    return data;
}

void SP_notifier(void *data)
{
    TP_header_t *hdr;
    SP_entry_t *ep;
    int i, num = 0;
    char buf[2048];
    char *p = buf;
    
    if (!data)
        return;
   
    debug(1,"enter %s: data %p\n",__FUNCTION__,data);

    hdr = (TP_header_t *)data;
    ep = (SP_entry_t *)hdr->data;

    for (i=0; i<hdr->number; i++) {
        if (!(ep[i].flag & SP_ENTRY_NEW)) 
            continue;
        debug(0, "Find new data\n"); 
        num = sprintf(p,"[%d]: %s, %s, %s->%s, %s, %s\n",i,ep[i].entity,ep[i].date,ep[i].from,ep[i].to,ep[i].action,ep[i].type);
        p += num;
    }

    *p = 0;

    if (p != buf)
        tp_send_mail(buf); 
    
    return;
}

int SP_init(void)
{
    SP_temp = calloc(1,sizeof(template_t));

    if (!SP_temp) {
        perror("Alloc SP template failed");
        return -1;
    }
    
    INIT_LIST_HEAD(&SP_temp->list);

    SP_temp->name[0] = 'S';
    SP_temp->name[1] = 'P';
    SP_temp->parser = SP_parser;
    SP_temp->filter = SP_filter;
    SP_temp->notifier = SP_notifier;

    register_template(SP_temp);
    return 0;
}
