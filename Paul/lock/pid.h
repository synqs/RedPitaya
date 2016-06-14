/**
 * @brief Red Pitaya PID Controller
 *
 * @Author Ales Bardorfer <ales.bardorfer@redpitaya.com>
 *         
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */

#ifndef __PID_H
#define __PID_H

typedef struct rp_pid_params_s {
    char  *name;
    float  value;
    int    fpga_update;
    int    read_only;
    float  min_val;
    float  max_val;
} rp_pid_params_t;

#define PARAMS_NUM        24
/* PID parameters */
#define PID_11_ENABLE     0
#define PID_11_RESET      1
#define PID_11_SP         2
#define PID_11_KP         3
#define PID_11_KI         4
#define PID_11_KD         5
#define PID_12_ENABLE     6
#define PID_12_RESET      7
#define PID_12_SP         8
#define PID_12_KP         9
#define PID_12_KI         10
#define PID_12_KD         11
#define PID_21_ENABLE     12
#define PID_21_RESET      13
#define PID_21_SP         14
#define PID_21_KP         15
#define PID_21_KI         16
#define PID_21_KD         17
#define PID_22_ENABLE     18
#define PID_22_RESET      19
#define PID_22_SP         20
#define PID_22_KP         21
#define PID_22_KI         22
#define PID_22_KD         23

#define PARAMS_PER_PID     6

int pid_init(void);
int pid_exit(void);

int pid_update(rp_pid_params_t *params, int len);

#endif // __PID_H
