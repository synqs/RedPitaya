/*
 * main.h
 *
 *  Created on: 05.04.2016
 * @author Paul Hill
 * @version 1.1
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "redpitaya/rp.h"

/*
 * brief description of the functionality of "laser stabilizer" app
 *
 * 1. Scan Mode:
 * 		-> generates a 10Hz triangle burst with amplitude and offset set by user
 * 		-> records the input during the burst
 * 		-> the user sets the "lock offset" value based on the recorded input
 * 		-> in scan mode the time window is fix: (25ms, 75ms) which is the falling edge of the triangle signal
 * 2. Lock Mode
 * 		-> generates a dc offset with the previously found "lock offset" voltage. This works as a offset for the pids
 * 		-> starts pids
 * 		-> the user can change the time window and time resolution
 * - main module handles requests from client / life cycle, updates pids and calculates important parameters for signal generation and acquisition (-> updates worker module). 
 * - worker module handles signal generation and acquisition asynchronously in a main loop function called "osc_worker_thread"
 *
 * Warning! On some RedPitayas creating the settings directory/file may fail (due different OS version with other file structure /permissions, ...)
 */

/* Parameters description structure for RP<->NGINX module.
 * In this application we also use the structure to sent commands from client to app
 *	 */
typedef struct rp_app_params_s {
    char  	*name;
    float 	value;
    int 	fpga_update;
    int    	read_only;
    float  	min_val;
    float	max_val;
} rp_app_params_t;

#define PARAMS_NUM        	42
#define OP_MODE_PARAM	  	0
#define MIN_GUI_PARAM     	1
#define MAX_GUI_PARAM     	2
#define TRIG_DLY_PARAM    	3
#define TRIG_LEVEL_PARAM  	4
#define TIME_RANGE_PARAM  	5
#define TIME_UNIT_PARAM   	6
#define TRIG_REC_DLY_PARAM 	7
#define EN_AVG_AT_DEC     	8
#define FORCEX_FLAG_PARAM 	9
#define GUI_RST_Y_RANGE   	10

/* AWG parameters */
#define TARGET_CHANNEL 	  	11				//also necessary for pid
#define GEN_SIG_AMP   	  	12
#define GEN_SIG_FREQ  	  	13
#define GEN_SIG_DC_OFF		14
/* PID parameters */
#define LOCK_OFF			15
#define PID_11_ENABLE     	16
#define PID_11_RESET      	17
#define PID_11_SP        	18
#define PID_11_KP         	19
#define PID_11_KI         	20
#define PID_11_KD         	21
#define PID_12_ENABLE     	22
#define PID_12_RESET      	23
#define PID_12_SP         	24
#define PID_12_KP         	25
#define PID_12_KI         	26
#define PID_12_KD         	27
#define PID_21_ENABLE     	28
#define PID_21_RESET      	29
#define PID_21_SP         	30
#define PID_21_KP         	31
#define PID_21_KI         	32
#define PID_21_KD         	33
#define PID_22_ENABLE     	34
#define PID_22_RESET      	35
#define PID_22_SP         	36
#define PID_22_KP         	37
#define PID_22_KI         	38
#define PID_22_KD         	39
/* CMD parameters */
#define CMD_CNT				40
#define CMD					41

/* Defines from which parameters on are AWG parameters (used in set_param() to
 * trigger update only on needed part - either Oscilloscope, AWG or PID */
#define PARAMS_AWG_PARAMS 11

/* Defines from which parameters on are PID parameters (used in set_param() to
 * trigger update only on needed part - either Oscilloscope, AWG or PID */
#define PARAMS_PID_PARAMS 15
#define PARAMS_PER_PID     6

#define PARAMS_CMD_PARAMS 40


/* Commands */
#define CMD_LOAD_SETTINGS 1
#define CMD_SAVE_SETTINGS 2


#define SETTINGS_DIR "/var/opt/stabilizer"
#define SETTINGS_FILE "/var/opt/stabilizer/settings_"			//final settings_file name will be settings_+date
#define SETTINGS_META_FILE "/var/opt/stabilizer/settings_meta"   //date of recent file


/* Output signals */
#define DEFAULT_SIGNAL_LENGTH 1024
#define MAX_SIGNAL_LENGTH 2048
#define SIGNALS_NUM   3

//PIDs have to wait until dc lock_offset is generated
#define PID_DELAY 1000000

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))


/* module entry points
 * must be implemented by all web-apps*/

int rp_app_init(void);

int rp_app_exit(void);

int rp_set_params(rp_app_params_t *p, int len);

int rp_get_params(rp_app_params_t **p);

int rp_get_signals(float ***s, int *sig_num, int *sig_len);

const char *rp_app_desc(void);


//executes command sent by client.
void execute_cmd(int cmd);

void release();

/* Waveform generator frequency limiter. */
float rp_gen_limit_freq(float freq, rp_waveform_t waveform);

char* concat(char *s1, char *s2);


#endif /* SRC_MAIN_H_ */
