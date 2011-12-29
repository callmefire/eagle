#include <stdlib.h>
#include "template.h"
#include "mail.h"
#include "debug.h"

extern int SP_init(void);

void tp_send_mail(char *body)
{
    char to[64] = "dchang@juniper.net,callmefire@139.com,callmefire@gmail.com";
    char from[64] = "\"Eagle\"<eagle@callmefire.com>";
    char draftname[64];
    char domain[16] = "jnpr";
    char subject[32] = "SP News";
    mail_param_t mp;

    sprintf(draftname,"%s/%ld",getenv("HOME"),random());

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
    debug(1, "#######################\n");
    SP_init();
}
