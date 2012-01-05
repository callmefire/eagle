#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "seed.h"
#include "template.h"
#include "debug.h"

static template_t *template = NULL;

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

static char *get_tbody_start(const char *buf, int len)
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

static char *get_tbody_end(char *start)
{
    char *p = NULL;

    p = strstr(start,TBODYEND);

    return p;
}

static int get_entry_num(char *start, char *end)
{
    char *p = start;
    char *q;
    int ret = 0;
    
    while ( (q = strstr(p,"<tr>")) && (q < end)) {
        p = q + 4;
        ret++;
   }
   
   debug(8,"enter %s: start %p, end %p, num %d\n",__FUNCTION__,start,end,ret-1);
   
   return (ret - 1);
}

static void extract_data(const char *start, const char *end, char *data)
{
    int i = 0;
    char *p = (char *)start;
    
    debug(8,"enter %s\n",__FUNCTION__);
    
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
            if ( (*p == ' ') && ((*(p+1) == ' ') || !i)) {
                ;
            } else {
                data[i] = *p;
                i++;
            }
        }
        p++;
    }
    
    data[i] = 0;
    
    debug(6,"extract data: %s\n",data);
    
    return;
}

static char *get_sp_entry(char *start, SP_entry_t *entry)
{
    char *tr_start;
    char *tr_end;
    char *td_start;
    char *td_end;

    if (!start || !entry)
        return NULL;

    tr_start = strstr(start,"<tr>");
    tr_end   = strstr(start,"</tr>");
    td_end   = tr_start + 4;

    td_start = strstr(td_end,"<td");
    if ( !td_start || (td_start > tr_end))
        return NULL;
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->entity);
    
    td_start = strstr(td_end,"<td");
    if ( !td_start || (td_start > tr_end))
        return NULL;
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->date);
    
    td_start = strstr(td_end,"<td");
    if ( !td_start || (td_start > tr_end))
        return NULL;
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->to);
     
    td_start = strstr(td_end,"<td");
    if ( !td_start || (td_start > tr_end))
        return NULL;
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->from);
     
    td_start = strstr(td_end,"<td");
    if ( !td_start || (td_start > tr_end))
        return NULL;
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->action);
     
    td_start = strstr(td_end,"<td");
    if ( !td_start || (td_start > tr_end))
        return NULL;
    td_start = strstr(td_start,">");
    td_start++;
    td_end = strstr(td_start,"</td>");
    extract_data(td_start, td_end, entry->type);
    
    return tr_end+5;
}

static void *parser(const char *buf, int len, void *seed)
{   
    char *tbody_start;
    char *tbody_end;
    char *p;
    int num = 0, i;
    TP_header_t *hdr = NULL;
    SP_entry_t *entry = NULL;
    
    debug(8,"enter %s\n",__FUNCTION__);
    
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
        if (!p)
            break;
    }
    
    return hdr;
}

static void dump_entry(TP_header_t *hdr)
{
    SP_entry_t *ep;
    int i;

    if (!hdr)
        return;

    ep = (SP_entry_t *)hdr->data;
    for (i=0; i<hdr->number; i++) {
        printf("<%d>: %s,%s,%s->%s,%s,%s\n",i,ep[i].entity,ep[i].date,ep[i].from,ep[i].to,ep[i].action,ep[i].type);    
    }
}

static void *filter(void *data, void *s) {
    seed_t *seed = s;
    TP_header_t *old;
    TP_header_t *new;
    SP_entry_t *np;
    SP_entry_t *op;
    int i,j,match,debug = 0;
    
    if (!data)
        return NULL;
        
    if (!seed->private) {
        seed->private = data;
        return NULL;
    }

    old = (TP_header_t *)seed->private;
    new = (TP_header_t *)data;

    np = (SP_entry_t *)new->data;

    for (i=0; i< new->number; i++) {
        op = (SP_entry_t *)old->data;
        match = 0;
        
        for (j=0; j<old->number; j++) {
            if ( !memcmp(np->entity, op->entity, strlen(np->entity)) &&
                 !memcmp(np->date,   op->date,   strlen(np->date))   &&
                 !memcmp(np->to,     op->to,     strlen(np->to))     &&
                 !memcmp(np->from,   op->from,   strlen(np->from))   &&
                 !memcmp(np->action, op->action, strlen(np->action)) &&
                 !memcmp(np->type,   op->type,   strlen(np->type)) ) {
                match = 1;
                break;
            }

            op++;
        }
        
        if (!match && strstr(np->action,"Revised") && strstr(np->type,"Foreign")) {
            np->flag |= SP_ENTRY_NEW;
            debug = 1;
        }
        np++;
    }

    if (debug) {
        dump_entry(old);
        dump_entry(new);
    }

    free(seed->private);
    seed->private = data;

    return data;
}

static void notifier(void *data)
{
    TP_header_t *hdr;
    SP_entry_t *ep;
    int i, num = 0;
    char *buf;
    char *p;
    
    if (!data)
        return;
   
    debug(8,"enter %s: data %p\n",__FUNCTION__,data);

    hdr = (TP_header_t *)data;
    ep = (SP_entry_t *)hdr->data;

    buf = (char *)calloc(1, hdr->number * (sizeof(SP_entry_t) + 32));
    if (!buf) {
        perror("No mem to alloc buf for notifier");
        return;
    }

    p = buf;

    for (i=0; i<hdr->number; i++) {
        if (!(ep[i].flag & SP_ENTRY_NEW)) 
            continue;
        debug(6, "Find new data\n"); 
        num = sprintf(p,"[%d]: %s, %s, %s->%s, %s, %s\n",i,ep[i].entity,ep[i].date,ep[i].from,ep[i].to,ep[i].action,ep[i].type);
        p += num;
    }

    *p = 0;

    if (p != buf)
        tp_send_mail(buf); 
   
    free(buf);
    return;
}

int SP_init(void)
{
    template = calloc(1,sizeof(template_t));

    if (!template) {
        perror("Alloc SP template failed");
        return -1;
    }
    
    INIT_LIST_HEAD(&template->list);

    template->name[0] = 'S';
    template->name[1] = 'P';
    template->parser = parser;
    template->filter = filter;
    template->notifier = notifier;

    register_template(template);
    return 0;
}
