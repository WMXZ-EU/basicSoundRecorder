/**
 * basicSoundRecorder
*/
#define START_MODE -1 // -1 wait for menu; 0 start with closed files

#include "menu.h"
#include "adc.h"
#include "acq.h"
#include "filing.h"
#include "mTime.h"

#include "utils.h"

/********************************* *********************************/
// Function prototypes and variables

int t_acq = 60;

uint32_t max_write=0;
uint32_t max_count=0;

uint32_t t_start;

/* check if we should close file */
int16_t checkToCloseFile(int16_t status, uint32_t t_acq)
{ if((status == OPENED) || (status == RUNNING))
  {
    //
    static uint32_t to =0;
    uint32_t tx=rtc_get();
    tx %= t_acq;
    if(tx<to) status = DOCLOSE; // time wrapped, file must be closed
    to=tx;
  } 
  return status;
}

void setup()
{
  while(!Serial) continue;

  Serial.println("basic Sound Recorder Version: " __DATE__  " " __TIME__ );

  storage_configure();
  
  adc_init();
  acq_init(FSAMP);

  acq_start(); 

  Serial.println("End of Setup");
}

void loop()
{ static int16_t status=START_MODE; 

  if(status==MUST_REBOOT) status=checkReboot(); // hapens only if microSD card write fails: reboot if space on disk

  // normal operation
  int16_t ch=menu();  // check if we have serial line command (0: no input; 1: start; -1: stop)

  if(ch>0 && status==STOPPED)  // was stopped, should run now 
  { 
    acq_start(); adcStatus();
  }  
  
  if(ch<0 && status>=CLOSED)  // was running, should stop now
  { status=MUSTSTOP;  acq_stop();  
  } 

  if(status >= CLOSED) // CLOSED or HIGHER
  {
    uint32_t to=millis();
    status = checkToCloseFile(status, (uint32_t) t_acq); // check if we reached file size or aquisition time
    //
    status = saveData(status);  
    //
    uint32_t mc = getCount();
    if(mc>max_count) max_count=mc;
    //
    uint32_t dt=millis()-to;
    if(max_write<dt) max_write=dt;
  }
}