
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include <time.h>

time_t timer;

float value = 0;

const char *rp_app_desc(void)
{
	time(&timer);
	fprintf(stderr, "app desc \t%s \n", ctime(&timer));
  return (const char *)"Red Pitaya osciloscope application.\n";
}

int rp_app_init(void)
{
	time(&timer);
	fprintf(stderr, "app init \t%s \n", ctime(&timer));
	system("LD_LIBRARY_PATH=/opt/redpitaya/lib /opt/redpitaya/bin/digital_led_bar 20");
  return 0;
}

int rp_app_exit(void)
{
	time(&timer);
	fprintf(stderr, "app exit \t%s \n",ctime(&timer));
	system("LD_LIBRARY_PATH=/opt/redpitaya/lib /opt/redpitaya/bin/digital_led_bar 0");
  return 0;
}

int rp_set_params(rp_app_params_t *p, int len)
{
	time(&timer);
	fprintf(stderr, "app set params \t%s \n",ctime(&timer));
	if(strcmp(p[0].name,"digital_led_bar")==0){
			value = p[0].value;
			if(value==1)
				system("LD_LIBRARY_PATH=/opt/redpitaya/lib /opt/redpitaya/bin/digital_led_bar 100");
	}
  return 0;
}

/* Returned vector must be free'd externally! */
int rp_get_params(rp_app_params_t **p)
{
	time(&timer);
	fprintf(stderr, "app get params \t%s \n",ctime(&timer));

	rp_app_params_t *p_copy = NULL;
	p_copy = (rp_app_params_t *)malloc((1+1) * sizeof(rp_app_params_t));
  if(p_copy == NULL)
    return -1;
	
	char* name = "digital_led_bar";
  int p_strlen = strlen(name);
  p_copy[0].name = (char *)malloc(p_strlen+1);				//nginx module tries to free this later
  strncpy((char *)&p_copy[0].name[0], &name[0],
                p_strlen);
  p_copy[0].name[p_strlen]='\0';

	p_copy[0].value = value;
	p_copy[1].name = NULL;

	*p = p_copy;  
	return 1;
}

int rp_get_signals(float ***s, int *sig_num, int *sig_len)
{
	time(&timer);
	fprintf(stderr, "app get signals \t%s \n",ctime(&timer));
  return 0;
}
