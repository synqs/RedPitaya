/*
 * worker.c
 *
 *  Created on: 07.04.2016
 * @author Paul Hill
 * @version 1.1
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include "main.h"
#include "worker.h"
#include "redpitaya/rp.h"


pthread_t *osc_thread_handler = NULL;
void *osc_worker_thread(void *args);

rp_osc_worker_state_t worker_ctrl_state = rp_osc_acquiring_state;

float **osc_signals = NULL;
float **tmp_signals = NULL;
int osc_signals_dirty = 0;
int osc_signal_size = 0;

int delay = 0, rec_delay = 0, time_range = 0, osc_params_init = 0, params_dirty = 0, op_mode, gen_channel = 0;
double tmin = 0, Tmax = 0, t_unit_factor = 0, trigger_level = 0, gen_sig_amp = 0, gen_sig_freq = 0, gen_sig_dc_off = 0, lock_off = 0;

pthread_mutex_t       osc_param_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t       osc_signals_mutex = PTHREAD_MUTEX_INITIALIZER;

double osc_smpl_periods[] = {1.0/125000000.0, 8.0/125000000.0, 64.0/125000000.0, 1024.0/125000000.0, 8192.0/125000000.0, 65536.0/125000000.0};

timeval_t t_last, t_curr, t_start;



int osc_worker_init(){
	int ret_val = 0;

	gettimeofday(&t_last, NULL);

	rp_cleanup_signals(&osc_signals);
	if(rp_create_signals(&osc_signals) < 0)
		return -1;

	rp_cleanup_signals(&tmp_signals);
	if(rp_create_signals(&tmp_signals) < 0){
		rp_cleanup_signals(&osc_signals);
		return -1;
	}

	osc_thread_handler = (pthread_t *)malloc(sizeof(pthread_t));
	if(osc_thread_handler == NULL) {
		rp_cleanup_signals(&osc_signals);
		rp_cleanup_signals(&tmp_signals);
		return -1;
	}
	ret_val = pthread_create(osc_thread_handler, NULL, osc_worker_thread, NULL);
	if(ret_val != 0) {
		rp_cleanup_signals(&osc_signals);
		rp_cleanup_signals(&tmp_signals);
	    fprintf(stderr, "pthread_create() failed: %s\n", strerror(errno));
	    return -1;
	}
	return 0;
}

int osc_worker_exit(){
	int ret_val = 0;

	worker_ctrl_state = rp_osc_abort_state;
	if(osc_thread_handler) {
		ret_val = pthread_join(*osc_thread_handler, NULL);
	    free(osc_thread_handler);
	    osc_thread_handler = NULL;
	}
	if(ret_val != 0) {
	    fprintf(stderr, "pthread_join() failed: %s\n", strerror(errno));
	}

	rp_cleanup_signals(&osc_signals);
	rp_cleanup_signals(&tmp_signals);

	return 0;
}

int osc_worker_update_params(rp_app_params_t *params){
	pthread_mutex_lock(&osc_param_mutex);
	delay = params[TRIG_DLY_PARAM].value;
	rec_delay = params[TRIG_REC_DLY_PARAM].value;
	time_range = params[TIME_RANGE_PARAM].value;
	op_mode = params[OP_MODE_PARAM].value;
	gen_channel = params[TARGET_CHANNEL].value;
	trigger_level = params[TRIG_LEVEL_PARAM].value;
	tmin = params[MIN_GUI_PARAM].value;
	t_unit_factor = pow(10, -3*(2 - params[TIME_UNIT_PARAM].value));
	Tmax = t_unit_factor * params[MAX_GUI_PARAM].value;
	gen_sig_freq = params[GEN_SIG_FREQ].value;
	gen_sig_amp = params[GEN_SIG_AMP].value;
	gen_sig_dc_off = params[GEN_SIG_DC_OFF].value;
	lock_off = params[LOCK_OFF].value;
	rp_osc_clean_signals();
	params_dirty = 1;
	osc_params_init = 1;
	pthread_mutex_unlock(&osc_param_mutex);
	return 0;
}


//main loop
void * osc_worker_thread(void *args){
	//local copies of params, so that params do not change in "working-loop"
	int lc_delay = 0, lc_rec_delay = 0, lc_time_range = 0, lc_op_mode = 0, lc_gen_channel;
	double lc_tmin = 0, lc_Tmax = 0, lc_t_unit_factor = 0, lc_trigger_level = 0, lc_gen_sig_amp = 0, lc_gen_sig_freq = 0, lc_gen_dc_off = 0, lc_lock_off = 0;
	uint32_t len = 0;
	double sig_duration = 0;
	while(worker_ctrl_state!=rp_osc_abort_state){
		pthread_mutex_lock(&osc_param_mutex);
		if(osc_params_init!=1 || worker_ctrl_state==rp_osc_idle_state){
			pthread_mutex_unlock(&osc_param_mutex);
			usleep(10000);
			continue;
		}
		if(params_dirty){		//update local params
			lc_delay = delay;
			lc_rec_delay = rec_delay;
			lc_time_range = time_range;
			lc_op_mode = op_mode;
			lc_gen_channel = gen_channel;
			lc_gen_dc_off = gen_sig_dc_off;
			lc_tmin = tmin;
			lc_Tmax = Tmax;
			lc_t_unit_factor = t_unit_factor;
			lc_trigger_level = trigger_level;
			lc_gen_sig_freq = gen_sig_freq;
			lc_gen_sig_amp = gen_sig_amp;
			lc_lock_off = lock_off;
			osc_prepare_generator(lc_op_mode, lc_gen_channel, lc_gen_sig_freq, lc_gen_sig_amp, lc_gen_dc_off, lc_lock_off, lc_Tmax, &sig_duration);
			params_dirty = 0;
		}
		pthread_mutex_unlock(&osc_param_mutex);

		rp_acq_decimation_t dec = lc_time_range;
		rp_AcqSetDecimation(dec);			//set sample rate

		rp_AcqSetTriggerLevel(lc_trigger_level);

		rp_AcqSetTriggerDelay(lc_delay - ADC_BUFFER_SIZE/2);    //specify amount of samples to be acquired. Bypass api-internal offset 

		uint32_t sleep_interval;
		float sampling_rate;
		rp_AcqGetSamplingRateHz(&sampling_rate);
		sleep_interval= ceil((((float)lc_delay) / sampling_rate) *1000000);

		//TODO op mode channel

		rp_acq_trig_src_t src =  RP_TRIG_SRC_NOW; //doesn't respect trigger level.

		rp_AcqStart();		//start reading samples
		gettimeofday(&t_start, NULL);

		if(lc_op_mode==0){
			if(sig_duration>0){//only when in scan mode and amplitude, frequency > 0
				src = RP_TRIG_SRC_AWG_PE;							//trigger on internal awg (triggers immediately after genTrigger is called)
																	//doesn't respect trigger level. needs burst mode / genTrigger to work!
				gettimeofday(&t_curr, NULL);
				//we have to wait until scan burst is finished and ACQ_START_DELAY after calling rp_AcqStart() is exceeded
				while(!params_dirty && worker_ctrl_state!=rp_osc_abort_state && ( osc_difftime(&t_curr, &t_last) < sig_duration || osc_difftime(&t_curr, &t_start) < ACQ_START_DELAY)){
					double sl_dur = MAX(sig_duration-osc_difftime(&t_curr, &t_last), ACQ_START_DELAY - osc_difftime(&t_curr, &t_start));
					if(sl_dur<1){
						usleep(ceil(sl_dur*1000000));
					}else{
						sleep(ceil(sl_dur));
					}
					gettimeofday(&t_curr, NULL);
				}
				if(params_dirty)
					continue;
				if(worker_ctrl_state==rp_osc_abort_state)
					break;
				rp_AcqSetTriggerSrc(src);				//arm trigger; will wait for trigger event and then acquire given amount of samples
				rp_GenTrigger(lc_gen_channel+1);		//start scan signal
				gettimeofday(&t_last, NULL);
			}else{
				rp_AcqSetTriggerSrc(src);		//arm trigger
			}
		}else{
			rp_AcqSetTriggerSrc(src);			//arm trigger
		}

		while(!params_dirty && worker_ctrl_state!=rp_osc_abort_state){
			rp_AcqGetTriggerSrc(&src);				//when all samples are acquired trigger source will be set to disabled
			if(src == RP_TRIG_SRC_DISABLED){
				break;
			}
			usleep(sleep_interval);
		}

		//stop scope when params are dirty or app is exiting and acquiring isn't finished yet
		rp_AcqGetTriggerSrc(&src);
		if(params_dirty){
			if(src!=RP_TRIG_SRC_DISABLED){
				rp_AcqStart();
				rp_AcqSetTriggerDelay(-ADC_BUFFER_SIZE/2);
				rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);
			}
			continue;
		}

		if(worker_ctrl_state==rp_osc_abort_state){
			if(src!=RP_TRIG_SRC_DISABLED){
				rp_AcqStart();
				rp_AcqSetTriggerDelay(-ADC_BUFFER_SIZE/2);
				rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);
			}
			break;
		}

		len = lc_rec_delay;

		//read out latest data
		rp_AcqGetLatestDataV(RP_CH_1, &len, &tmp_signals[1][0]);

		if(params_dirty)
			continue;
		if(worker_ctrl_state==rp_osc_abort_state)
			break;

		rp_AcqGetLatestDataV(RP_CH_2, &len, &tmp_signals[2][0]);

		//generate time signal
		int i;
		float dT = osc_smpl_periods[lc_time_range];
		//float m = 4*lc_gen_sig_amp*lc_gen_sig_freq;
		for(i=1;i<lc_rec_delay;++i){
			tmp_signals[0][i] = /*(lc_op_mode==0 && lc_gen_sig_freq>0 && lc_gen_sig_amp>0) ? lc_gen_sig_amp - m*i*dT :*/ lc_tmin+i*dT/lc_t_unit_factor;
		}
		if(params_dirty)
			continue;
		if(worker_ctrl_state==rp_osc_abort_state)
			break;

		//replace old signals
		rp_osc_set_signals(tmp_signals, lc_rec_delay);
	}
	return 0;
}

/*
 * enable output signal and calculate duration of the scan-signal burst
 *
 * generates dc offset, in lock mode
 * sets up generator for triangle burst, in scan mode (burst is then triggered by calling genTrigger(..) elsewhere)
 */
int osc_prepare_generator(int lc_op_mode, int lc_gen_channel, float lc_gen_sig_freq, float lc_gen_sig_amp, float lc_gen_dc_off, float lc_lock_off, float lc_Tmax, double *sig_duration){
    rp_channel_t ch = lc_gen_channel==0 ? RP_CH_1 : RP_CH_2;
    //disable unused channel
    if(ch==RP_CH_1){
    	rp_GenAmp(RP_CH_2, 0);
        rp_GenOutDisable(RP_CH_2);
    }else{
    	rp_GenAmp(RP_CH_1, 0);
        rp_GenOutDisable(RP_CH_1);
    }
    if(lc_op_mode==1){//lock mode
    	float amp;
    	rp_waveform_t form;
    	rp_GenGetWaveform(ch, &form);
    	if(form==RP_WAVEFORM_TRIANGLE)				//mode changed from scan mode to lock mode -> reset generator
    		rp_GenReset();
    	rp_GenGetAmp(ch, &amp);
    	if(lc_lock_off==0){
    		rp_GenOutDisable(ch);
    		return 0;
    	}
    	if(amp==lc_lock_off)
    		return 1;
        rp_GenMode(ch, RP_GEN_MODE_CONTINUOUS);
    	rp_GenWaveform(ch, RP_WAVEFORM_DC);
    	rp_GenAmp(ch, 0);						//cannot generate negative dc output (amp has to be 1>=amp>=0 )
    	rp_GenOffset(ch, lc_lock_off);			//use offset instead
    	rp_GenOutEnable(ch);
    	return 1;
    }

    rp_GenReset();

    if(lc_gen_sig_amp<=0 || lc_gen_sig_freq<=0){
        rp_GenOutDisable(ch);
        *sig_duration = -1;
        return 0;
    }
    rp_GenFreq(ch, lc_gen_sig_freq);
    rp_GenAmp(ch, lc_gen_sig_amp/2.0);
    rp_GenOffset(ch, lc_gen_dc_off);
    rp_GenWaveform(ch, RP_WAVEFORM_TRIANGLE);
    rp_GenMode(ch, RP_GEN_MODE_BURST);
    rp_GenTriggerSource(ch, RP_GEN_TRIG_SRC_INTERNAL);

    int count = ceil(lc_Tmax*lc_gen_sig_freq);
    rp_GenBurstCount(ch, count+1);
    *sig_duration = 1.1 *count/lc_gen_sig_freq + osc_smpl_periods[0]*10;  //burst duration + small offset		//calculate duration of scan-signal burst

    rp_GenOutEnable(ch);

    return 1;
}

int osc_get_signals(float ***signals, int *sig_size){
	float **s = *signals;
	pthread_mutex_lock(&osc_signals_mutex);
	if(osc_signals_dirty == 0) {//just old signals
		*sig_size = osc_signal_size;
	    pthread_mutex_unlock(&osc_signals_mutex);
	    return 0;
	}

	memcpy(&s[0][0], &osc_signals[0][0], sizeof(float)*osc_signal_size);
	memcpy(&s[1][0], &osc_signals[1][0], sizeof(float)*osc_signal_size);
	memcpy(&s[2][0], &osc_signals[2][0], sizeof(float)*osc_signal_size);

	*sig_size = osc_signal_size;

	osc_signals_dirty = 0;

	pthread_mutex_unlock(&osc_signals_mutex);
	return 0;
}

/*
 * @brief returns the time differences between t1 and t0
 */
double osc_difftime(timeval_t *t1, timeval_t *t0){
	return ( t1->tv_sec - t0->tv_sec + (t1->tv_usec - t0->tv_usec)/1000000.0 );
}

int rp_osc_clean_signals(void)
{
    pthread_mutex_lock(&osc_signals_mutex);
    osc_signals_dirty = 0;
    pthread_mutex_unlock(&osc_signals_mutex);
    return 0;
}

int rp_osc_set_signals(float **source, int length)
{
    pthread_mutex_lock(&osc_signals_mutex);

    memcpy(&osc_signals[0][0], &source[0][0], sizeof(float)*length);
    memcpy(&osc_signals[1][0], &source[1][0], sizeof(float)*length);
    memcpy(&osc_signals[2][0], &source[2][0], sizeof(float)*length);

    //we have a new signal
    osc_signals_dirty = 1;
    osc_signal_size = length;

    pthread_mutex_unlock(&osc_signals_mutex);

    return 0;
}

int rp_create_signals(float ***a_signals)
{
    int i;
    float **s;

    s = (float **)malloc(SIGNALS_NUM * sizeof(float *));
    if(s == NULL) {
        return -1;
    }
    for(i = 0; i < SIGNALS_NUM; i++)
        s[i] = NULL;

    for(i = 0; i < SIGNALS_NUM; i++) {
        s[i] = (float *)malloc(MAX_SIGNAL_LENGTH * sizeof(float));
        if(s[i] == NULL) {
            rp_cleanup_signals(a_signals);
            return -1;
        }
        memset(&s[i][0], 0, MAX_SIGNAL_LENGTH * sizeof(float));
    }
    *a_signals = s;

    return 0;
}

void rp_cleanup_signals(float ***a_signals)
{
    int i;
    float **s = *a_signals;

    if(s) {
        for(i = 0; i < SIGNALS_NUM; i++) {
            if(s[i]) {
                free(s[i]);
                s[i] = NULL;
            }
        }
        free(s);
        *a_signals = NULL;
    }
}

