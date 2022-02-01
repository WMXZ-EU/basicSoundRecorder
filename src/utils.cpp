#include "utils.h"
#include "core_pins.h"

uint32_t getTeensySerial(void);
uint32_t SerNum;

/* following from https://github.com/sstaub/TeensyID/blob/master/TeensyID.cpp */
#if defined ARDUINO_TEENSY40 || defined ARDUINO_TEENSY41
  uint32_t getTeensySerial(void) 
  {
    uint32_t num;
    num = HW_OCOTP_MAC0 & 0xFFFFFF;
    return num;
  }

  // from https://forum.pjrc.com/threads/33443-How-to-display-free-ram?p=275013&viewfull=1#post275013
  extern unsigned long _heap_start; 
  extern unsigned long _heap_end;
  extern char *__brkval;

  int freeram() {  return (char *)&_heap_end - __brkval; }

  #define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
  #define CPU_RESTART_VAL 0x5FA0004
  #define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL)

#else
  uint32_t getTeensySerial(void) 
  {
    uint32_t num = 0;
    __disable_irq();
    #if defined(HAS_KINETIS_FLASH_FTFA) || defined(HAS_KINETIS_FLASH_FTFL)
      FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
      FTFL_FCCOB0 = 0x41;
      FTFL_FCCOB1 = 15;
      FTFL_FSTAT = FTFL_FSTAT_CCIF;
      while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) ; // wait
      num = *(uint32_t *)&FTFL_FCCOB7;
    #elif defined(HAS_KINETIS_FLASH_FTFE)
      kinetis_hsrun_disable();
      FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
      *(uint32_t *)&FTFL_FCCOB3 = 0x41070000;
      FTFL_FSTAT = FTFL_FSTAT_CCIF;
      while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)) ; // wait
      num = *(uint32_t *)&FTFL_FCCOBB;
      kinetis_hsrun_enable();
    #endif
    __enable_irq();
    return num & 0xFFFFFF;
  }
#endif

#include "usb_serial.h"
Print *stdPrint = &Serial;

extern "C"
{
  int _write(int file, const void *buf, size_t len) {
    // https://forum.pjrc.com/threads/28473-Quick-Guide-Using-printf()-on-Teensy-ARM
    Print *out;
  
    // Send both stdout and stderr to stdPrint
    if (file == stdout->_file || file == stderr->_file) {
      out = stdPrint;
    } else {
      out = (Print *)file;
    }
  
    if (out == nullptr) {
      return len;
    }
  
    // Don't check for len == 0 for returning early, in case there's side effects
    return out->write((const uint8_t *)buf, len);
  }
  
}

