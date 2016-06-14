/**
 * @brief RedPitaya Web-Shell main module
 * @author Paul Hill
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include <sys/types.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <unistd.h>
#include <signal.h>


static rp_app_params_t rp_main_params[PARAMS_NUM] = {
		{"psswd",0,0,0,0,0},
		{"cmnd",0,0,0,0,0}
};

const char *rp_app_desc(void)
{
    return (const char *)"RedPitaya Web-Shell application.\n";
}

int rp_app_init(void)
{
	fprintf(stderr, "RedPitaya Web-Shell init.\n");
	FILE* out_fd = fopen("/tmp/shell_out.txt","w");
	fclose(out_fd);
    return 0;
}

int rp_app_exit(void)
{
	fprintf(stderr, "exit RedPitaya Web-Shell.\n");
    return 0;
}

int rp_set_params(rp_app_params_t *p, int len)
{
	if(len!=2 || p[0].name==NULL || p[1].name==NULL){
		fprintf(stderr, "\nwrong amount of parameters or parameter not defined: %d; %s : %f \t%s : %f\n",len,p[0].name,p[0].status, p[1].name, p[1].status);
		return -1;
	}
	if(!loggedIn){
		int l = strlen(p[0].name);
		psswd_raw = (char *)malloc(l+1);
		if(psswd_raw==NULL){
			fprintf(stderr, "could not allocate memory\n");
			return -1;
		}
		strcpy(psswd_raw,p[0].name);
		psswd_raw[l]='\0';
		if(auth_sys_user("root",psswd_raw)==0){
			loggedIn = 1;
			rp_main_params[0].status=1;
			fprintf(stderr, "RedPitaya Web-Shell: logged in.\n");
			return 0;
		}else{
			free(psswd_raw);
			rp_main_params[0].status=0;
			return 0;
		}
	}
	if(strcmp(psswd_raw, p[0].name)!=0){
		rp_main_params[0].status=0;
		fprintf(stderr, "wrong password\n");
		return 0;
	} else
		rp_main_params[0].status=1;
	int fcl = strlen(p[1].name)+strlen(c2f);
	char *full_cmd = (char *)malloc(fcl+1);
	if(full_cmd==NULL){
		rp_main_params[0].status=0;
		return 0;
	}
	strcpy(full_cmd,p[1].name);
	strcat(full_cmd,c2f);
	fprintf(stderr, "RedPitaya Web-Shell: execute command ");
	fprintf(stderr, full_cmd);
	fprintf(stderr, "\n");
	system(full_cmd);
    return 0;
}

/* Returned vector must be free'd externally! */
int rp_get_params(rp_app_params_t **p)
{
	//fprintf(stderr, "RedPitaya Web-Shell: get params.\n");
	rp_app_params_t *copy = NULL;
	copy = (rp_app_params_t *)malloc((PARAMS_NUM +1)*sizeof(rp_app_params_t));
	if(copy==NULL)
		return -1;
	int i;
	for(i=0;i<PARAMS_NUM;++i){
		int sl = strlen(rp_main_params[i].name);
		copy[i].name = (char *)malloc(sl+1);
		strncpy((char *)&copy[i].name[0], rp_main_params[i].name, sl);
		copy[i].name[sl]='\0';
		copy[i].status=rp_main_params[i].status;
	}
	copy[PARAMS_NUM].name = NULL;
	*p=copy;
    return PARAMS_NUM;
}

int rp_get_signals(float ***s, int *sig_num, int *sig_len)
{
	//fprintf(stderr, "RedPitaya Web-Shell: get signals.\n");
	if(*s == NULL)
		return -1;
	*sig_num = 3;
	*sig_len = SIGNAL_LENGTH;
	char *line;
	size_t line_len = 0;
	float **r = *s;
	int l = 0;
	int k=0;
	for(k=0;k<3;++k){
		int i = 0;
		for(i=0;i<SIGNAL_LENGTH;++i){
			r[k][i]=-10;
		}
	}
	FILE* out_fd = fopen("/tmp/shell_out.txt","r");
	if(out_fd==NULL){
		r[0][0]=-1;							//error
		return 0;
	}
	int i = 0;
	while (i<out_index && (getline(&line, &line_len, out_fd) != -1)) {
		++i;
	}
	if((l=getline(&line,&line_len,out_fd))==-1){
		r[0][0]=0;							//nothing to read
		fclose(out_fd);
		return 0;
	}
	++out_index;
	int j=0;
	for(j=0;j<SIGNAL_LENGTH-1 && j<(l+1);++j){
		r[1][j]=(float)((int)line[j]);
	}
	r[1][SIGNAL_LENGTH-1]=(float)((int)'\0');
	if(getline(&line,&line_len,out_fd)!=-1)
		r[0][0]=2;								//lines left
	else
		r[0][0]=1;								//all lines are read
	fclose(out_fd);
	if(line!=NULL)
		free(line);
    return 0;
}

extern int auth_sys_user (const char*username, const char*password) {
    struct passwd *pw;
    struct spwd *sp;
    char *encrypted, *correct;

    pw = getpwnam(username);
    endpwent();

    if (!pw) return 1; //user doesn't exist

    sp = getspnam(pw->pw_name);
    endspent();

    correct = sp ? sp->sp_pwdp : pw->pw_passwd;
    encrypted = crypt(password, correct);
    return strcmp(encrypted, correct) ? 2 : 0;  // bad pw=2, success=0
}
