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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pid.h"
#include "fpga_pid.h"

/**
 * GENERAL DESCRIPTION:
 *
 * The code below sets the PID parameters to the user-specified values.
 * A direct translation from GUI PID parameters to the FPGA registers.
 *
 * The PID Controller algorithm itself is implemented in FPGA.
 * There are 4 independent PID controllers, connecting each input (IN1, IN2)
 * to each output (OUT1, OUT2):
 *
 *                 /-------\       /-----------\
 *   IN1 -----+--> | PID11 | ------| SUM & SAT | ---> OUT1
 *            |    \-------/       \-----------/
 *            |                            ^
 *            |    /-------\               |
 *            ---> | PID21 | ----------    |
 *                 \-------/           |   |
 *                                     |   |
 *                                     |   |
 *                                     |   |
 *                 /-------\           |   |
 *            ---> | PID12 | --------------
 *            |    \-------/           |
 *            |                        ˇ
 *            |    /-------\       /-----------\
 *   IN2 -----+--> | PID22 | ------| SUM & SAT | ---> OUT2
 *                 \-------/       \-----------/
 *
 */


rp_pid_params_t rp_main_params[PARAMS_NUM+1] = {
    /******************************************/
    /* PID Controller parameters from here on */
    /******************************************/

    { /* pid_NN_enable - Enables/closes or disables/open PID NN loop:
       *    0 - PID disabled (open loop)
       *    1 - PID enabled (closed loop)    */
        "pid_11_enable", 0, 1, 0, 0, 1 },
    { /* pid_NN_rst - Reset PID NN integrator:
        *    0 - Do not reset integrator
        *    1 - Reset integrator            */
        "pid_11_rst", 0, 1, 0, 0, 1 },
    { /* pid_NN_sp - PID NN set-point in [ADC] counts. */
        "pid_11_sp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kp - PID NN proportional gain Kp in [ADC] counts. */
        "pid_11_kp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_ki - PID NN integral gain     Ki in [ADC] counts. */
        "pid_11_ki",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kd - PID NN derivative gain   Kd in [ADC] counts. */
        "pid_11_kd",  0, 1, 0, -8192, 8191 },

    { /* pid_NN_enable - Enables/closes or disables/open PID NN loop:
       *    0 - PID disabled (open loop)
       *    1 - PID enabled (closed loop)    */
        "pid_12_enable", 0, 1, 0, 0, 1 },
    { /* pid_NN_rst - Reset PID NN integrator:
        *    0 - Do not reset integrator
        *    1 - Reset integrator            */
        "pid_12_rst", 0, 1, 0, 0, 1 },
    { /* pid_NN_sp - PID NN set-point in [ADC] counts. */
        "pid_12_sp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kp - PID NN proportional gain Kp in [ADC] counts. */
        "pid_12_kp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_ki - PID NN integral gain     Ki in [ADC] counts. */
        "pid_12_ki",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kd - PID NN derivative gain   Kd in [ADC] counts. */
        "pid_12_kd",  0, 1, 0, -8192, 8191 },

    { /* pid_NN_enable - Enables/closes or disables/open PID NN loop:
       *    0 - PID disabled (open loop)
       *    1 - PID enabled (closed loop)    */
        "pid_21_enable", 0, 1, 0, 0, 1 },
    { /* pid_NN_rst - Reset PID NN integrator:
        *    0 - Do not reset integrator
        *    1 - Reset integrator            */
        "pid_21_rst", 0, 1, 0, 0, 1 },
    { /* pid_NN_sp - PID NN set-point in [ADC] counts. */
        "pid_21_sp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kp - PID NN proportional gain Kp in [ADC] counts. */
        "pid_21_kp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_ki - PID NN integral gain     Ki in [ADC] counts. */
        "pid_21_ki",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kd - PID NN derivative gain   Kd in [ADC] counts. */
        "pid_21_kd",  0, 1, 0, -8192, 8191 },

    { /* pid_NN_enable - Enables/closes or disables/open PID NN loop:
       *    0 - PID disabled (open loop)
       *    1 - PID enabled (closed loop)    */
        "pid_22_enable", 0, 1, 0, 0, 1 },
    { /* pid_NN_rst - Reset PID NN integrator:
        *    0 - Do not reset integrator
        *    1 - Reset integrator            */
        "pid_22_rst", 0, 1, 0, 0, 1 },
    { /* pid_NN_sp - PID NN set-point in [ADC] counts. */
        "pid_22_sp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kp - PID NN proportional gain Kp in [ADC] counts. */
        "pid_22_kp",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_ki - PID NN integral gain     Ki in [ADC] counts. */
        "pid_22_ki",  0, 1, 0, -8192, 8191 },
    { /* pid_NN_kd - PID NN derivative gain   Kd in [ADC] counts. */
        "pid_22_kd",  0, 1, 0, -8192, 8191 },

	{ /* Must be last! */
		NULL, 0.0, -1, -1, 0.0, 0.0 }
};

/*----------------------------------------------------------------------------------*/
/** @brief Initialize PID Controller module
 *
 * A function is intended to be called within application initialization. It's purpose
 * is to initialize PID Controller module and to calculate maximal voltage, which can be
 * applied on DAC device on individual channel.
 *
 * @retval     -1 failure, error message is reported on standard error
 * @retval      0 successful initialization
 */

int pid_init(void)
{
    if(fpga_pid_init() < 0) {
        return -1;
    }
    pid_update(rp_main_params, PARAMS_NUM);
    return 0;
}


/*----------------------------------------------------------------------------------*/
/** @brief Cleanup PID COntroller module
 *
 * A function is intended to be called on application's termination. The main purpose
 * of this function is to release allocated resources...
 *
 * @retval      0 success, never fails.
 */
int pid_exit(void)
{
    fpga_pid_exit();

    return 0;
}


/*----------------------------------------------------------------------------------*/
/**
 * @brief Update PID Controller module towards actual settings.
 *
 * A function is intended to be called whenever one of the following settings on each PID
 * sub-controller is modified:
 *    - Enable
 *    - Integrator reset
 *    - Set-point
 *    - Kp
 *    - Ki
 *    - Kd
 *
 * @param[in] params  Pointer to overall configuration parameters
 * @retval -1 failure, error message is repoted on standard error device
 * @retval  0 succesful update
 */
int pid_update(rp_pid_params_t *p, int len)
{
    int i;
    for(i = 0; i < len && p[i].name != NULL; i++) {
    	int p_idx = -1;
        int j = 0;
            /* Search for correct parameter name in defined parameters */
        while(rp_main_params[j].name != NULL) {
        	int p_strlen = strlen(p[i].name);

            if(p_strlen != strlen(rp_main_params[j].name)) {
            	j++;
                continue;
            }
            if(!strncmp(p[i].name, rp_main_params[j].name, p_strlen)) {
                p_idx = j;
                break;
            }
            j++;
        }

        if(p_idx == -1) {
            fprintf(stderr, "Parameter %s not found, ignoring it\n", p[i].name);
            continue;
        }

        if(rp_main_params[p_idx].read_only)
            continue;

        if(rp_main_params[p_idx].min_val > p[i].value) {
            fprintf(stderr, "Incorrect parameters value: %f (min:%f), "
                        " correcting it\n", p[i].value, rp_main_params[p_idx].min_val);
            p[i].value = rp_main_params[p_idx].min_val;
        } else if(rp_main_params[p_idx].max_val < p[i].value) {
            fprintf(stderr, "Incorrect parameters value: %f (max:%f), "
                        " correcting it\n", p[i].value, rp_main_params[p_idx].max_val);
            p[i].value = rp_main_params[p_idx].max_val;
        }
    }

    pid_param_t pid[NUM_OF_PIDS] = {{ 0 }};
    uint32_t ireset = 0;

    for (i = 0; i < NUM_OF_PIDS; i++) {
        /* PID enabled? */
        if (p[PID_11_ENABLE + i * PARAMS_PER_PID].value == 1) {
            pid[i].kp = (int)p[PID_11_KP + i * PARAMS_PER_PID].value;
            pid[i].ki = (int)p[PID_11_KI + i * PARAMS_PER_PID].value;
            pid[i].kd = (int)p[PID_11_KD + i * PARAMS_PER_PID].value;
        }

        g_pid_reg->pid[i].setpoint = (int)p[PID_11_SP + i * PARAMS_PER_PID].value;
        g_pid_reg->pid[i].kp = pid[i].kp;
        g_pid_reg->pid[i].ki = pid[i].ki;
        g_pid_reg->pid[i].kd = pid[i].kd;

        if (p[PID_11_RESET + i * PARAMS_PER_PID].value == 1) {
            ireset |= (1 << i);
        }
    }

    g_pid_reg->configuration = ireset;

    return 0;
}

