/**
 * @brief main module
 * @author Paul Hill
 * @version 1.1
 */
#include <stdlib.h>
#include <stdio.h>
#include <libio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <math.h>
#include "main.h"
#include "redpitaya/rp.h"
#include "worker.h"
#include "pid.h"


pthread_mutex_t rp_main_params_mutex = PTHREAD_MUTEX_INITIALIZER;
static rp_app_params_t rp_main_params[PARAMS_NUM+1] = {

	{ /* operation mode
	   * 0 - scan
	   * 1 - lock */
		"op_mode", 0, 1, 0, 0, 1},
    { /* min_gui_time   */
        "xmin", 25, 1, 0, 0, +10000000 },
    { /* max_gui_time   */
        "xmax", 75, 1, 0, 0, +10000000 },
	{ /* trig_delay: amount of data after trigger event that needs to be acquired to capture interesting time window (xmin, xmax); represents time window (0, xmax)  */
		"trig_delay", DEFAULT_SIGNAL_LENGTH, 1, 1, 0, +10000000 },
	{ /* trig_level : Trigger level, expressed in normalized 1V  */
		"trig_level", 0, 1, 0,     -2,     +2 },
    { /* time_range:
       *  decimation:
       *    0 - 1x
       *    1 - 8x
       *    2 - 64x
       *    3 - 1kx
       *    4 - 8kx
       *    5 - 65kx   */
        "time_range", 0, 1, 1,         0,         5 },
    { /* time_unit_used:
       *    0 - [us]
       *    1 - [ms]
       *    2 - [s]     */
        "time_units", 1, 0, 1,         0,         2 },
	{ /* trig_rec_delay: amount of data to be captured (given time window (xmin, xmax) after trigger event)  */
		"trig_rec_delay", 0, 1, 1,     0,     100000 },
    { /* en_avg_at_dec:
           *    0 - disable
           *    1 - enable */
        "en_avg_at_dec", 0, 1, 0,      0,         1 },
    { /* forcex_flag:
       * Server sets this flag when X axis time units change
       * Client checks this flag, when set the server's xmin:xmax define the visualization range
       *    0 - normal operation
       *    1 - Server forces xmin, xmax  */
        "forcex_flag", 0, 0, 0, 0, 1 },
    { /* gui_reset_y_range - Maximum voltage range [Vpp] with current settings
       * This parameter is calculated by application and is read-only for
       * client.
       */
        "gui_reset_y_range", 28, 0, 1, 0, 2000 },

    /********************************************************/
    /* Arbitrary Waveform Generator parameters from here on */
    /********************************************************/

    { /* gen_sig_ch - Selects the channel for the scan signal and dc offset for pids:
       *    0 - channel 1
       *    1 - channel 2*/
        "target_channel", 0, 1, 0, 0, 1 },
    { /* gen_sig_amp - Amplitude in [Vpp] */
        "gen_sig_amp", 0, 1, 0, 0, 2.0 },
    { /* gen_sig_freq - Frequency in [Hz] */
        "gen_sig_freq", 10, 1, 1, 0, 50e6 },
	{/* gen_dc_off */
		"gen_sig_dc_off", 0, 1, 0, -1, 1},

    /******************************************/
    /* PID Controller parameters from here on */
    /******************************************/

	{ /* lock_off offset for pid output on target channel*/
		"lock_off", 0, 1, 0, -1, 1},

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

		/******************************************/
		/* Command parameters from here on */
		/******************************************/
		{ /* cmd_cnt index of recent command */
			"cmd_cnt", 0, 0, 0, 0, 1000 },
		{ /* cmd encoded command */
			"cmd", 0, 0, 0, 0, 100 },

    { /* Must be last! */
        NULL, 0.0, -1, -1, 0.0, 0.0 }
};

int params_init = 0, mode_change = 0;

double smpl_periods[] = {1.0/125000000.0, 8.0/125000000.0, 64.0/125000000.0, 1024.0/125000000.0, 8192.0/125000000.0, 65536.0/125000000.0};


const char *rp_app_desc(void)
{
	return (const char *)"Laser stabilizer application.\n";
}

int rp_app_init(void)
{
	fprintf(stderr, "laser stabilizer init.\n");

	struct stat st = {0};

	if (stat(SETTINGS_DIR, &st) == -1) {
	    if(mkdir(SETTINGS_DIR, 0700)==-1)						//create directory for settings file if not existing
	    	return -1;											//may fail on an other RedPitaya (other OS version, manipulated file structure,..)
	}

	if(rp_Init()!=RP_OK){
		release();
		return -1;
	}
	if(rp_AcqReset()!=RP_OK){
		release();
		return -1;
	}
	if(rp_GenReset()!=RP_OK){
		release();
		return -1;
	}
	if(osc_worker_init()==-1){
		release();
		return -1;
	}
	if(pid_init()==-1){
		release();
		return -1;
	}
    rp_set_params(&rp_main_params[0], PARAMS_NUM); 						//initialize with default params
    return 0;
}

void release(){
	rp_Release();
}

int rp_app_exit(void)
{
	fprintf(stderr, "exit laser stabilizer.\n");
	osc_worker_exit();
	pid_exit();
	release();
    return 0;
}

/*
 * calculate scope params: suitable sampling rate and amount of samples to be acquired based on the given time window (xmin, xmax)
 */
void transform_acq_params(rp_app_params_t *p)
{
	fprintf(stderr, "laser stabilizer tap.\n");
	p[FORCEX_FLAG_PARAM].value = 1;								//force gui to use calculated xmin, xmax value

	if(p[OP_MODE_PARAM].value==0){								//fixed time settings in scan mode
		p[MAX_GUI_PARAM].value = 75;
		p[MIN_GUI_PARAM].value = 25;
		p[TIME_UNIT_PARAM].value = 1;		//ms
	}

	double xmax = p[MAX_GUI_PARAM].value;
	double xmin = p[MIN_GUI_PARAM].value;

	int time_unit = p[TIME_UNIT_PARAM].value;
	double t_unit_factor = pow(10, -3*(2 - time_unit));

	xmax *= t_unit_factor;
	xmin *= t_unit_factor;
	double dT = xmax-xmin;

	//rec_delay: amount of samples to be delivered to user; only depends on dT; see above
	int rec_delay = DEFAULT_SIGNAL_LENGTH, i=0;

	if(dT/DEFAULT_SIGNAL_LENGTH<smpl_periods[0]){	//wanted resolution too high
		rec_delay = ceil(dT/smpl_periods[0])+1;		//round up +1, so we take "0s"-sample into account
		p[TIME_RANGE_PARAM].value = 0;
	}else if(dT/DEFAULT_SIGNAL_LENGTH>smpl_periods[5]){	//wanted resolution too low
		rec_delay = MIN(MAX_SIGNAL_LENGTH, dT/smpl_periods[5] +1);
		p[TIME_RANGE_PARAM].value = 5;
	}else{
		//find best fitting sampling rate
		for(i=1;i<6;++i){
			if(smpl_periods[i]>dT/DEFAULT_SIGNAL_LENGTH)
				break;
		}
		int d = MIN(MAX_SIGNAL_LENGTH, dT/smpl_periods[i-1] +1);
		int d2 = ceil(dT/smpl_periods[i])+1;
		if(fabs(d*smpl_periods[i-1]-dT) > fabs(d2*smpl_periods[i]-dT)){
			p[TIME_RANGE_PARAM].value = i;
			rec_delay = d2;
		}else{
			p[TIME_RANGE_PARAM].value = i-1;
			rec_delay = d;
		}
	}

	//calc new time_unit for gui
	if(time_unit==0 && xmax/t_unit_factor>=1000){			 //µs
		time_unit = 1;
		t_unit_factor = pow(10, -3*(2 - time_unit));
	}else if(time_unit==1){									//ms
		if(xmax/t_unit_factor>=1000)
			time_unit = 2;
		else if(xmax/t_unit_factor<1)
			time_unit = 0;
		t_unit_factor = pow(10, -3*(2 - time_unit));
	}else if(time_unit==2 && xmax/t_unit_factor<1){			//s
		time_unit = 1;
		t_unit_factor = pow(10, -3*(2 - time_unit));
	}
	p[MAX_GUI_PARAM].value = dT/t_unit_factor + xmin/t_unit_factor;//+ p[MIN_GUI_PARAM].value;
	p[TIME_UNIT_PARAM].value = time_unit;

	//overall amount of samples that have to be acquired after trigger event occurs; see above
	p[TRIG_DLY_PARAM].value = xmin/smpl_periods[(int)p[TIME_RANGE_PARAM].value] + rec_delay;

	p[TRIG_REC_DLY_PARAM].value = rec_delay;
}

int rp_set_params(rp_app_params_t *p, int len)
{
	fprintf(stderr, "laser stabilizer set params.\n");
    int i;
    int params_change = 0;
    int pid_params_change = 0;
    int exec_cmd = 0;

    if(len > PARAMS_NUM) {
        fprintf(stderr, "Too many parameters, max=%d\n", PARAMS_NUM);
        return -1;
    }

    pthread_mutex_lock(&rp_main_params_mutex);

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

        if(rp_main_params[p_idx].value != p[i].value) {
            if (p_idx < PARAMS_PID_PARAMS)
                params_change = 1;
            if(p_idx >= PARAMS_PID_PARAMS && p_idx<PARAMS_CMD_PARAMS)
                pid_params_change = 1;
            if(p_idx==OP_MODE_PARAM || p_idx==TARGET_CHANNEL){
            	if(p[i].value==1)															//lock mode
            		mode_change = 1;														//delay pid update
            	pid_params_change = params_change = 1;
            }
            if(p_idx==LOCK_OFF)
            	params_change = 1;
            if(p_idx==CMD_CNT){
            	if(rp_main_params[p_idx].value<p[i].value)
            		exec_cmd = 1;
            	else
            		continue;
            }
        }
        if(rp_main_params[p_idx].min_val > p[i].value) {
            fprintf(stderr, "Incorrect parameters value: %f (min:%f), "
                    " correcting it\n", p[i].value, rp_main_params[p_idx].min_val);
            p[i].value = rp_main_params[p_idx].min_val;
        } else if(rp_main_params[p_idx].max_val < p[i].value) {
            fprintf(stderr, "Incorrect parameters value: %f (max:%f), "
                    " correcting it\n", p[i].value, rp_main_params[p_idx].max_val);
            p[i].value = rp_main_params[p_idx].max_val;
        }
        rp_main_params[p_idx].value = p[i].value;
    }
    rp_main_params[GUI_RST_Y_RANGE].value = 2.136014F;				//for lv use with probe attenuation 1 only
    															//otherwise, correct value must be calculated using calib_params

    if(exec_cmd && rp_main_params[CMD].value==CMD_LOAD_SETTINGS){	//load params before calculate scope params
    	int old_mode = rp_main_params[OP_MODE_PARAM].value;
    	int old_target_ch = rp_main_params[TARGET_CHANNEL].value;
    	execute_cmd(rp_main_params[CMD].value);

    	//new params have to be updated
    	pid_params_change = params_change = 1;
    	if(rp_main_params[OP_MODE_PARAM].value==1 && (rp_main_params[OP_MODE_PARAM].value!=old_mode || rp_main_params[TARGET_CHANNEL].value != old_target_ch))
    		mode_change = 1;
    }

    /* calculate important params based on given time window and update worker thread  */
    if(params_change || (params_init == 0)) {

        transform_acq_params(rp_main_params);					//calculate scope params
        params_init = 1;

        // Correct frequencies if needed
        rp_main_params[GEN_SIG_FREQ].value = rp_gen_limit_freq(rp_main_params[GEN_SIG_FREQ].value, RP_WAVEFORM_TRIANGLE);

        //handles acquiring and signal generator
        osc_worker_update_params(rp_main_params);
    }

    if (pid_params_change) {

    	if(rp_main_params[OP_MODE_PARAM].value==0){
    		pid_disable();
    		if(exec_cmd && rp_main_params[CMD].value==CMD_SAVE_SETTINGS)
    			execute_cmd(rp_main_params[CMD].value);
    		pthread_mutex_unlock(&rp_main_params_mutex);
    		return 0;
    	}
    	if(mode_change==1){
    	   usleep(PID_DELAY);				//pids have to wait until lock_offset dc output is active
    	   mode_change = 0;
    	}
        if(pid_update(&rp_main_params[0], rp_main_params[TARGET_CHANNEL].value) < 0) {
        	pthread_mutex_unlock(&rp_main_params_mutex);
            return -1;
        }
    }

    if(exec_cmd && rp_main_params[CMD].value==CMD_SAVE_SETTINGS)			//save all params
        execute_cmd(rp_main_params[CMD].value);

    pthread_mutex_unlock(&rp_main_params_mutex);

    return 0;
}

void execute_cmd(int cmd){
	++rp_main_params[CMD_CNT].value;
	int i;
	FILE *f, *m;
	char *file_name = NULL;
	time_t t;
	switch(cmd){
	case CMD_LOAD_SETTINGS :
		//get date of recent file
		m = fopen(SETTINGS_META_FILE, "r");
		if(m==NULL)
			return;
		char date[100];
		if(fgets(date, 100, m)==NULL ){
			fclose(m);
			return;
		}
		fclose(m);

		//generate filenames : settings_+date
		file_name = concat(SETTINGS_FILE, date);
		if(file_name==NULL){
			return;
		}

		//load settings
		f = fopen(file_name, "r");
		free(file_name);
		if(f==NULL)
			return;

		for(i=0;i<PARAMS_CMD_PARAMS;++i){
				if(fread(&rp_main_params[i].value, sizeof(float), 1, f)!=1){
					fclose(f);
					return;
				}
		}
		fclose(f);
		break;
	case CMD_SAVE_SETTINGS :
		//save current time in meta file
		t = time(0);
		m = fopen(SETTINGS_META_FILE, "w");
		if(m==NULL)
			return;
		if(fprintf(m, ctime(&t))<0){
			fclose(m);
			return;
		}
		fclose(m);

		//generate file_name : settings_+date
		file_name = concat(SETTINGS_FILE, ctime(&t));
		if(file_name==NULL)
			return;

		//save settings
		f = fopen(file_name, "w");
		free(file_name);
		if(f==NULL)
			return;

		for(i=0;i<PARAMS_CMD_PARAMS;++i){
				if(fwrite(&rp_main_params[i].value, sizeof(float), 1, f)!=1){
					fclose(f);
					return;
				}
		}
		fclose(f);
		break;
	}
}

/* Returned vector must be free'd externally! */
int rp_get_params(rp_app_params_t **p)
{
    rp_app_params_t *p_copy = NULL;
    int i;

    p_copy = (rp_app_params_t *)malloc((PARAMS_NUM+1) * sizeof(rp_app_params_t));
    if(p_copy == NULL)
        return -1;

    pthread_mutex_lock(&rp_main_params_mutex);
    for(i = 0; i < PARAMS_NUM; i++) {
        int p_strlen = strlen(rp_main_params[i].name);
        p_copy[i].name = (char *)malloc(p_strlen+1);
        strncpy((char *)&p_copy[i].name[0], &rp_main_params[i].name[0],
                p_strlen);
        p_copy[i].name[p_strlen]='\0';

        p_copy[i].value       = rp_main_params[i].value;
        p_copy[i].fpga_update = rp_main_params[i].fpga_update;
        p_copy[i].read_only   = rp_main_params[i].read_only;
        p_copy[i].min_val     = rp_main_params[i].min_val;
        p_copy[i].max_val     = rp_main_params[i].max_val;
    }
    pthread_mutex_unlock(&rp_main_params_mutex);
    p_copy[PARAMS_NUM].name = NULL; 					//prudent! Otherwise NGINX does not know array length

    *p = p_copy;
    return PARAMS_NUM;
}

int rp_get_signals(float ***s, int *sig_num, int *sig_len)
{
   if(*s == NULL)
        return -1;

    *sig_num = SIGNALS_NUM;
    *sig_len = 0;

    return osc_get_signals(s, sig_len);
/*

	if(*s == NULL)
	   return -1;

    *sig_num = SIGNALS_NUM;

    pthread_mutex_lock(&rp_main_params_mutex);

    *sig_len = rp_main_params[TRIG_REC_DLY_PARAM].value;
    float dT = smpl_periods[(int)rp_main_params[TIME_RANGE_PARAM].value];
    float t_unit_factor = pow(10, -3*(2 - rp_main_params[TIME_UNIT_PARAM].value));
    float min = rp_main_params[MIN_GUI_PARAM].value;

    pthread_mutex_unlock(&rp_main_params_mutex);

    int i,k=0;
    for(i=0;i<*sig_len;++i)
        (*s)[k][i] = min+dT*i/t_unit_factor;
    for(k=1;k<*sig_num;++k){
    	for(i=0;i<*sig_len;++i)
    		(*s)[k][i] = sin((min*t_unit_factor+dT*i)*100000);
    }
    return 0;*/
}


float rp_gen_limit_freq(float freq, rp_waveform_t waveform)
{
    if(freq < 0) {
        freq = 0;
    } else {
        switch(waveform) {
        case RP_WAVEFORM_SINE:
            /* Sine */
            if(freq > 50e6)
                freq = 50e6;
            break;
        case RP_WAVEFORM_SQUARE:
            /* Square */
            if(freq > 20e6)
                freq = 20e6;
            break;
        case RP_WAVEFORM_TRIANGLE:
            /* Triangle */
            if(freq > 25e6)
                freq = 25e6;
            break;
        default:
        	if(freq > 50e6)
        		freq = 50e6;
        	break;
        }
    }

    return freq;
}

char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator; free externally !
    if(result==NULL)
    	return NULL;
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

