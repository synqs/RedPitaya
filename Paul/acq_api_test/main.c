/*
 * main.c
 *
 *  Created on: 22.03.2016
 *      Author: Paul Hill
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include "redpitaya/rp.h"

int main(){
	if(rp_Init() != RP_OK){
		fprintf(stderr, "Rp api init failed!\n");
		return 0;
	}

	printf("Be sure that OUT2 is connected to IN2.\n");
	printf("trigger source:\n\t0: RP_TRIG_SRC_DISABLED\n\t4: RP_TRIG_SRC_CHB_PE\ntrigger state:\n\t%d: RP_TRIG_STATE_TRIGGERED\n\t%d: RP_TRIG_STATE_WAITING\n",RP_TRIG_STATE_TRIGGERED,RP_TRIG_STATE_WAITING);

	uint32_t len = 2000;		//samples to be acquired after trigger

	//prepare scope
	rp_AcqReset();
	rp_AcqSetDecimation(RP_DEC_64);
	rp_AcqSetTriggerLevel(0);
	rp_AcqSetTriggerDelay(len-ADC_BUFFER_SIZE/2);  //we want to acquire exactly 2000 samples after trigger event, so we have to bypass the internal offset

	//generate 'triangle' on channel 2
	rp_GenReset();
	rp_GenFreq(RP_CH_2, 200.0);
	rp_GenAmp(RP_CH_2, 1.0);
	rp_GenWaveform(RP_CH_2, RP_WAVEFORM_TRIANGLE);
	rp_GenOutEnable(RP_CH_2);


	uint32_t pos, tr_pos;
	rp_acq_trig_src_t src = 0;
	rp_acq_trig_state_t state = 0;
	uint32_t adclen = ADC_BUFFER_SIZE;			//size of buffer

	printf("lets look how the write pointer moves after rp_AcqReset()\n");
	int k = 2;
	while(k>0){
		usleep(10);
		--k;
		rp_AcqGetWritePointerAtTrig(&tr_pos);
		rp_AcqGetWritePointer(&pos);
		rp_AcqGetTriggerSrc(&src);
		rp_AcqGetTriggerState(&state);
		printf("\tpos: %d; pos when triggered: %d; trigger source: %d; trigger state: %u\n",pos, tr_pos, src, state);
	}

	printf("start acquiring samples\n");
	rp_AcqStart();

	printf("lets look how the write pointer moves\n");
	k=3;
	while(k>0){
		usleep(10);
		--k;
		rp_AcqGetWritePointerAtTrig(&tr_pos);
		rp_AcqGetWritePointer(&pos);
		rp_AcqGetTriggerSrc(&src);
		rp_AcqGetTriggerState(&state);
		printf("\twrite pointer: %d; write pointer at trigger: %d; trigger source: %d; trigger state: %d\n",pos, tr_pos, src, state);
	}

	printf("arm scope\n");
	rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHB_PE);

	printf("wait for trigger (0V posedge on Channel2)\n");
	while(true){
		rp_AcqGetWritePointerAtTrig(&tr_pos);
		rp_AcqGetWritePointer(&pos);
		rp_AcqGetTriggerSrc(&src);
		rp_AcqGetTriggerState(&state);
		printf("\twrite pointer: %d; write pointer at trigger: %d; trigger source: %d; trigger state: %d\n",pos, tr_pos, src, state);
		if(src==RP_TRIG_SRC_DISABLED)
			break;						//all samples are acquired
		usleep(1);
	}
	printf("scope is done with acquiring (2000) samples. Does the write pointer move?\n");

	k=2;
	while(k>0){
		usleep(10);
		--k;
		rp_AcqGetWritePointerAtTrig(&tr_pos);
		rp_AcqGetWritePointer(&pos);
		rp_AcqGetTriggerSrc(&src);
		rp_AcqGetTriggerState(&state);
		printf("\twrite pointer: %d; write pointer at trigger: %d; trigger source: %d; trigger state: %d\n",pos, tr_pos, src, state);
	}

	//we can now read out the buffer

	//latest samples
	float *latest = (float *)malloc(len * sizeof(float));
	rp_AcqGetLatestDataV(RP_CH_2, &len, latest);

	//oldest samples
	float *oldest = (float *)malloc(len * sizeof(float));
	rp_AcqGetOldestDataV(RP_CH_2, &len, oldest);

	//all samples
	float *all = (float *)malloc(adclen * sizeof(float));
	rp_AcqGetDataV(RP_CH_2, 0, &adclen, all);


	//works only with small len. For large len it's likely to 'overflow'
	printf("\nread out interesting part of buffer\ni\t\tdata\t\tlatest\t\toldest\n");
	int n;
	for(n=tr_pos-5;n<pos+len+6 && n<adclen;++n){
		if(n==tr_pos+7)
			printf("...\n");
		if(n<pos-5 &&n>tr_pos+5)
			continue;
		if(n==pos+len-4)
			printf("...\n");
		if(n>pos+5 &&n<pos-4+len)
			continue;
		if(n==pos)
			printf("~~~~~~~~\nwrite pointer\n");
		if(n==tr_pos)
			printf("~~~~~~~~\nwrite pointer at trigger\n");
		if(n<pos && n>pos-len-1)
			printf("%d\t\t%f\t\t%f\t\t-\n",n, all[n],latest[n-pos+len]);
		else if(n>pos && n<pos+1+len)
			printf("%d\t\t%f\t\t-\t\t%f\n",n, all[n],oldest[n-pos-1]);
		else
			printf("%d\t\t%f\t\t-\t\t-\n",n, all[n]);
		if(n==pos || n==tr_pos)
			printf("~~~~~~~~\n");
	}
	free(latest);
	free(oldest);
	free(all);
	rp_Release();
}
