#include <mail.h>

static int __send_mail(char *draftname)
{
    char cmd[FILENAME_MAX];

    sprintf(cmd,"send %s",draftname);
	if (system(cmd) < 0) {
        perror("System");
        return -1;
    }

    return 0;
}

int send_mail(mail_param_t *mp)
{
	FILE *fp = fopen(mp->draftname,"w");
	
    if(!fp) {
		perror("Draft:");
        return -1;
	}

	fprintf(fp,"MIME-Version: 1.0\n");
    if (mp->charset)
	    fprintf(fp,"Content-type: text/plain; charset=%s\n",mp->charset);
	fprintf(fp,"Domain:%s\n", mp->domain?mp->domain:"");
	fprintf(fp,"From:%s\n", mp->from?mp->from:"");
	fprintf(fp,"To:%s\n", mp->to?mp->to:"");
	fprintf(fp,"cc:%s\n", mp->cc?mp->cc:"");
	fprintf(fp,"bcc:%s\n", mp->bcc?mp->bcc:"");
	fprintf(fp,"Subject:%s\n", mp->subject?mp->subject:"");
	fprintf(fp,"\n");
    fprintf(fp,"%s\n",mp->body?mp->body:"");
	fprintf(fp,"\n");
	fprintf(fp,"%s\n", mp->sig?mp->sig:"");
	fprintf(fp,"%s\n", mp->date?mp->date:"");
		
    fsync(fileno(fp));
	fclose(fp);

    __send_mail(mp->draftname);

    return 0;
}

#if 0
int main(int argc,char **argv) {
	char from[64]="\"A\"<A@test.net>";
    char to[64]="\"B\"<B@test.net>";
	char domain[32]="test";
	char subject[64]="title";
    char body[64]="test body";
    char draftname[64];
    mail_param_t mp;

    sprintf(draftname,"%s/\0",getenv("HOME"));
    strcat(draftname,get_draft_name());
    set_draft_name(draftname);
    printf("%s\n",get_draft_name());

    memset(&mp,0,sizeof(mp));

    mp.from    = from;
    mp.to      = to;
    mp.subject = subject;
    mp.domain  = domain;
    mp.body    = body;

    if (fill_mail(&mp) < 0) {
        perror("Fill mail");
        return -1;
    }

    if (send_mail() < 0) {
        perror("Send mail");
        return -1;
    }

    return 0;
}

#endif
