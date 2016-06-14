/*
 * worker.h
 *
 *  Created on: 07.04.2016
 * @author Paul Hill
 * @version 1.1
 */

#ifndef SRC_WORKER_H_
#define SRC_WORKER_H_

/*
 * handles signal acquisation and generation
 */

#include "main.h"
#include <sys/time.h>

typedef enum rp_osc_worker_state_e {
    rp_osc_idle_state,
    rp_osc_abort_state,
	rp_osc_acquiring_state
} rp_osc_worker_state_t;

typedef struct timeval timeval_t;


#define ACQ_START_DELAY 	1					//minimum delay between acqStart() and genTrigger. [s]

int osc_worker_init();
int osc_worker_exit(void);
int osc_worker_update_params(rp_app_params_t *params);

int  rp_create_signals(float ***a_signals);
void rp_cleanup_signals(float ***a_signals);
/* removes 'dirty' flags */
int rp_osc_clean_signals(void);

double osc_difftime(timeval_t *t1, timeval_t *t0);

/* Returns:
 *  0 - new signals (dirty signal) are copied to the output
 *  1 - no new signals available (dirty signal was not set - we need to wait)
 */
int osc_get_signals(float ***signals, int *sig_idx);
/* Fills the output signal structure from temp one after calculation is done
 * and marks it dirty
 */
int rp_osc_set_signals(float **source, int index);

int osc_prepare_generator(int lc_op_mode, int lc_gen_channel, float lc_gen_sig_freq, float lc_gen_sig_amp, float lc_gen_dc_off, float lc_lock_off, float lc_Tmax, double *sig_duration);



#endif /* SRC_WORKER_H_ */
