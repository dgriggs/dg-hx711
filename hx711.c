/* 
 gurov was here, use this code, or don't, whatever, I don't care. If you see a giant bug with a billion legs, please let me know so it can be squashed

*/


#include <stdio.h>
#include "gb_common.h"
#include <math.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define CLOCK_PIN	3
#define DATA_PIN	2
#define N_SAMPLES	16
#define SPREAD		10

#define SCK_ON  (GPIO_SET0 = (1 << CLOCK_PIN))
#define SCK_OFF (GPIO_CLR0 = (1 << CLOCK_PIN))
#define DT_R    (GPIO_IN0  & (1 << DATA_PIN))

void           reset_converter(void);
unsigned long  read_cnt(int debug);
void           set_gain(int r);
void           setHighPri (void);
unsigned long  get_reading(float calibration_factor);
void	       tare(void);


long offset=0;

void setHighPri (void)
{
  struct sched_param sched ;

  memset (&sched, 0, sizeof(sched)) ;

  sched.sched_priority = 10 ;
  if (sched_setscheduler (0, SCHED_FIFO, &sched))
    printf ("Warning: Unable to set high priority\n") ;
}


void setup_gpio()
{
  INP_GPIO(DATA_PIN);
  INP_GPIO(CLOCK_PIN);  OUT_GPIO(CLOCK_PIN);
  SCK_OFF;

//   GPIO_PULL = 0;
//   short_wait();
//   GPIO_PULLCLK0 = 1 << DATA_PIN;
 //  short_wait();
//   GPIO_PULL = 0;
//   GPIO_PULLCLK0 = 0;

/*   GPIO_PULL = 2;
   short_wait();
   GPIO_PULLCLK0 = 1 << DATA_PIN;
   short_wait();
   GPIO_PULL = 0;
   GPIO_PULLCLK0 = 0;*/
}

void unpull_pins()
{
   GPIO_PULL = 0;
//   short_wait();
   GPIO_PULLCLK0 = 1 << DATA_PIN;
//   short_wait();
   GPIO_PULL = 0;
   GPIO_PULLCLK0 = 0;
} // unpull_pins

int main(int argc, char **argv)
{
  long reading = 0;
  float calibration_factor = 1;
  int b;
	
  setHighPri();
  setup_io();
  setup_gpio();
  reset_converter();
	
  if (argc == 2) 
  {
    if (strcmp(argv[1],"0") == 0)
    {
      printf("Invalid calibration factor. Cannot divide by zero.\n");
      exit(1);
    }
    else
    {
    calibration_factor = fabsf(atof(argv[1]));
    offset = get_reading(calibration_factor);
    }
  }
  else
  {
    printf("Please enter 1 argument - a non-zero positive float value for calibration_factor.\n");
    exit(1);
  }	
  while(true)
  {
    reading = get_reading(calibration_factor);
    printf("Current Reading: %d\n", reading);
    nanosleep((const struct timespec[]){{1, 0L}}, NULL);
  }
  unpull_pins();
  restore_io();
}
	
unsigned long get_reading(float calibration_factor) 
{
  int i, j;
  long tmp=0;
  long tmp_avg=0;
  long tmp_avg2=0;
  long output=0;
  float filter_low, filter_high;
  float spread_percent = SPREAD / 100.0 /2.0;
  int nsamples=N_SAMPLES;
  long samples[nsamples];

  // get the dirty samples and average them
  for(i=0;i<nsamples;i++) {
  	reset_converter();
  	samples[i] = read_cnt(0);
  	tmp_avg += samples[i];
  }

  tmp_avg = tmp_avg / nsamples;

  tmp_avg2 = 0;
  j=0;

  filter_low =  (float) tmp_avg * (1.0 - spread_percent);
  filter_high = (float) tmp_avg * (1.0 + spread_percent);

  //printf("Filter low threshold: %d -- Filter high threshold: %d\n", (int) filter_low, (int) filter_high);

  for(i=0;i<nsamples;i++) {
	if ((samples[i] < filter_high && samples[i] > filter_low) || 
            (samples[i] > filter_high && samples[i] < filter_low) ) {
		tmp_avg2 += samples[i];
	        j++;
	}
  }
  if (j == 0) {
    printf("No data met filter requirements.\n");
    exit(0);
  }
  output = ((( (float) tmp_avg2 / (float) j) / calibration_factor) - (float) offset);
  printf("average within %.2f percent: %d from %d samples, original: %d\n", spread_percent*100, (tmp_avg2 / j) - offset, j, tmp_avg - offset);
  return output;
}



void reset_converter(void) {
	SCK_ON;
	usleep(60);
	SCK_OFF;
	usleep(60);
}

void set_gain(int r) {
	int i;

// r = 0 - 128 gain ch a
// r = 1 - 32  gain ch b
// r = 2 - 63  gain ch a

	while( DT_R ); 

	for (i=0;i<24+r;i++) {
		SCK_ON;
		SCK_OFF;
	}
}


unsigned long read_cnt(int debug) {
	long count;
	int i;
	int b;


  count = 0;


  while( DT_R ); 
	b++;
	b++;
	b++;
	b++;

  for(i=0;i<24	; i++) {
	SCK_ON;
        count = count << 1;
	b++;
	b++;
	b++;
	b++;
        SCK_OFF;
	b++;
	b++;
        if (DT_R > 0 ) { count++; }
//	b++;
//	b++;
  }


	SCK_ON;
	b++;
	b++;
	b++;
	b++;
	SCK_OFF;
	b++;
	b++;
	b++;
	b++;
//  count = ~0x1800000 & count;
//  count = ~0x800000 & count;


  if (count & 0x800000) {
	count |= (long) ~0xffffff;
  }
/*
  // If things are broken this will show actual data
  printf("Debug mode with calibration factor = '1'\n"); 
  for (int i=31; i>=0; i--) {
    printf("%d ", ((count) & ( 1 << i )) != 0 );
    printf("n: %10d     -  ", count);
    printf("\n");
  }
	*/
  return (count);
}


