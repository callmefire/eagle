#ifndef __MAIL_H__
#define __MAIL_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct mail_param {
    char *draftname;
    char *charset;
    char *from;
    char *to;
    char *cc;
    char *bcc;
    char *subject;
    char *domain;
    char *body;
    char *date;
    char *sig;
};

typedef struct mail_param mail_param_t;

extern int send_mail(mail_param_t *mp);

#endif
