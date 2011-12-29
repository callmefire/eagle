#ifndef __TEMPLATE_H__

#define __TEMPLATE_H__

#include "list.h"

typedef void * (*parser_t)(const char *, int, void *);
typedef void * (*filter_t)(void *, void *);
typedef void   (*notifier_t)(void *);

typedef struct {
    struct list_head list;
    char name[16];
    parser_t   parser;
    filter_t   filter;
    notifier_t notifier;
} template_t;

typedef struct TP_header {
    int number;
    char data[0];
} TP_header_t;

extern struct list_head temp_list;


extern void parse(const char *, int len, void *);
extern void parser_init(void);
extern int register_template(template_t *);
extern void template_init(void);

extern void tp_send_mail(char *body);

#endif
