
#ifndef __MAIN_H
#define __MAIN_H

/* Parameters description structure */
typedef struct rp_app_params_s {
    char  *name;
    float  value;
   	int    fpga_update;
    int    read_only;
    float  min_val;
    float  max_val;
} rp_app_params_t;



/* module entry points */
int rp_app_init(void);
int rp_app_exit(void);
int rp_set_params(rp_app_params_t *p, int len);
int rp_get_params(rp_app_params_t **p);
int rp_get_signals(float ***s, int *sig_num, int *sig_len);

#endif /*  __MAIN_H */
