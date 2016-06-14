/*
 * main.c
 *
 *  Created on: 22.03.2016
 *      Author: paul
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "pid.h"
#include "fpga_pid.h"
#include "redpitaya/rp.h"

rp_pid_params_t params[PARAMS_NUM] = {
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
        "pid_22_kd",  0, 1, 0, -8192, 8191 }
};

/** Print usage information */
void usage() {

    const char *format =
        "PID 11 & PID 21 CONTROLLER\n"
        "\n"
        "Usage:    target(11) Kp(11) Ki(11) Kd(11) Kp(21) Ki(21) Kd(21) \n"
        "\n";

    fprintf( stderr, format);
}

/*
 * @brief 	disables PID 1->1 and 1->2 and sets target and factors to zero
 */
void reset();

/*
 * @brief 	sets parameters for PID Input Ch1 to Output Channel 1/2
 *
 * @target 	target voltage
 * @kp11	proportional factor for PID 1->1
 * @ki11	integral factor for PID 1->1
 * @kd11	differential factor for PID 1->1
 * @kp21	proportional factor for PID 1->2
 * @ki21	integral factor for PID 1->2
 * @kd21	differential factor for PID 1->2
 */
void relock(int target, int Kp11, int Ki11, int Kd11, int Kp21, int Ki21, int Kd21);

/*
 * @brief acquires samples from channel 1 or 2
 *
 * @len size of acquired data
 * @dec decimation   
 * @channel input channel to read from
 * @avg if true acquired data will be averaged
 */
float acquire(uint32_t len, rp_acq_decimation_t dec, rp_channel_t channel, int avg);

int main (int argc, char **argv) {

	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
		printf("Rp api init failed!\n");
		return 0;
	}
	if (argc==8) {
		//init
		pid_init();
		rp_AcqReset();					


		//read arguments
		/*int target= atoi(argv[1]);
		int Kp11= atoi(argv[2]);
		int Ki11= atoi(argv[3]);
		int Kd11= atoi(argv[4]);

		int Kp21= atoi(argv[5]);
		int Ki21= atoi(argv[6]);
		int Kd21= atoi(argv[7]);

		relock(target,Kp11,Ki11,Kd11,Kp21,Ki21,Kd21);

		float lock=acquire(1000, RP_DEC_1, RP_CH_1, 1);*/

		//float test;
		//printf("value : %lf\n", lock);
		//int i=0;
		/*while (i==0){
			test=acquire(1000);
			if (abs(test-lock)>10) {
				reset();
				usleep(1000);
				relock(target,Kp11,Ki11,Kd11,Kp21,Ki21,Kd21);
				usleep(1000);
				lock=acquire(1000);
				fprintf(stderr,"relock");
			}

		}*/
		//Zum Testen von acquire und des pids: Input 1 und Output 2 verbinden. Programm starten mit Arg. 0 0 0 0 0 100 0
		//Spannung sollte dann nahezu auf 0 geregelt sein.
		rp_GenReset();
		rp_GenFreq(RP_CH_2, 20000.0);
		rp_GenAmp(RP_CH_2, 0.8000);
		rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
		rp_GenOutEnable(RP_CH_2);
		float lock=acquire(100, RP_DEC_8, RP_CH_2, 1);

				//float test;
		printf("value : %lf\n", lock);
		sleep(20);
		pid_exit();
		rp_Release();
	}
	else {
		fprintf(stderr,"wrong nummber of arguments \n");
		usage();
	}
	return 0;

}

//Ich habe nicht verstanden warum du reset_pids() aufrufst. Reicht es nicht einfach alles auf null zu setzen?
//Dann muss man den PID auch nur einmal intitialisieren.
//Außerdem konnte ich es so nicht kompilieren, wegen mehrfach Inklusion (man kann nicht fpga_pid.c und fpga_pid.h gleichzeitig inkludieren)

void reset () {
	params[PID_11_ENABLE].value=0;
	params[PID_11_SP].value=0;
	params[PID_11_KP].value=0;
	params[PID_11_KI].value=0;
	params[PID_11_KD].value=0;
	params[PID_11_RESET].value=1;

	params[PID_21_ENABLE].value=0;
	params[PID_21_SP].value=0;
	params[PID_21_KP].value=0;
	params[PID_21_KI].value=0;
	params[PID_21_KD].value=0;
	params[PID_21_RESET].value=1;

	pid_update(params, PARAMS_NUM);

	usleep(500);

  params[PID_11_RESET].value=0;
  params[PID_21_RESET].value=0;
	
	pid_update(params, PARAMS_NUM);
}

void relock (int target, int Kp11, int Ki11, int Kd11, int Kp21, int Ki21, int Kd21) {
	reset();
	usleep(500);

	params[PID_11_ENABLE].value=1;
	params[PID_11_SP].value=target;
	params[PID_11_KP].value=Kp11;
	params[PID_11_KI].value=Ki11;
	params[PID_11_KD].value=Kd11;
	params[PID_11_RESET].value=0;

	params[PID_21_ENABLE].value=1;
	params[PID_21_SP].value=target;
	params[PID_21_KP].value=Kp21;
	params[PID_21_KI].value=Ki21;
	params[PID_21_KD].value=Kd21;
	params[PID_21_RESET].value=0;

	pid_update(params, PARAMS_NUM);
}

float acquire(uint32_t len, rp_acq_decimation_t dec, rp_channel_t channel, int avg){
	if(len>16384){
		fprintf(stderr, "Invalid buffer size!\n");
		printf("Invalid buffer size!\n");
		rp_Release();
		exit(0);
		return 0;
	}
	float *buff = (float *)malloc(len * sizeof(float));
	if(buff==NULL){
		fprintf(stderr, "Failed to allocate buffer!\n");
		printf("Failed to allocate buffer!\n");
		rp_Release();
		exit(0);
		return 0;
	}
	rp_AcqSetDecimation(dec);
	rp_AcqSetTriggerDelay(len -ADC_BUFFER_SIZE/2);		
	rp_AcqStart();	
									
	uint32_t sleep_interval;
	float sampling_rate;
	rp_AcqGetSamplingRateHz(&sampling_rate);
	sleep_interval= ceil((((float)len) / sampling_rate) *1000000);		//calculate time that acquiring will need in µs.
																		//Be sure to take offset into account.
	//print sampling_rate for debugging
	printf("sampling rate [µs], [s]: %d, %f\n",sleep_interval, ((float)len)/sampling_rate);

	rp_acq_trig_src_t src = RP_TRIG_SRC_NOW;	//trigger immediately
	rp_AcqSetTriggerSrc(src);					//scope is now 'armed' and waits for trigger event

	while(1){
		//printf("loop\n");
		rp_AcqGetTriggerSrc(&src);
	    if(src == RP_TRIG_SRC_DISABLED){ 		//trigger event occurred and specified amount of samples are written.
	    	break;
	    }
    	usleep(sleep_interval); 				//prudent. Otherwise, the core will be blocked by the while-loop.
    											//For optimization: Count loop passes/time and try to adjust sleep_interval
	}

	rp_AcqGetLatestDataV(channel, &len, buff);	//read the latest samples
	//print buffer for debugging
	/*int k;
	for(k=0;k<len;++k){
		printf("buff[%d] = %f\n",k,buff[k]);
	}*/
	if(avg){
		int i;
		float sum = 0;
		for(i=0;i<len;++i){
			sum += buff[i];
		}
		free(buff);
		return sum/len;
	}
	float ret = buff[0];
	free(buff);
	return ret;
}
