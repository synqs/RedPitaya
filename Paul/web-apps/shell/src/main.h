/**
 * @brief RedPitaya Web-Shell main module
 * @author Paul Hill
 */

#ifndef __MAIN_H
#define __MAIN_H

#define PARAMS_NUM        2

typedef struct rp_app_params_s {
    char  	*name; 			//param name  //it holds the login password and command in our app
    float 	status; 		//param value //it holds the status in our app since we do not need to exchange "real" parameters
    int 	fpga_update;	
    int    	read_only;
    float  	min_val;
    float	max_val;
} rp_app_params_t;

const int SIGNAL_LENGTH = 1024;

char *psswd_raw = NULL;
short loggedIn;
char *c2f = " >> /tmp/shell_out.txt 2>> /tmp/shell_out.txt";
int out_index = 0;


/* module entry points
 * must be implemented by all web-apps*/

int rp_app_init(void);

int rp_app_exit(void);

int rp_set_params(rp_app_params_t *p, int len);

int rp_get_params(rp_app_params_t **p);

int rp_get_signals(float ***s, int *sig_num, int *sig_len);

const char *rp_app_desc(void);



int auth_sys_user (const char*username, const char*password);

#endif
