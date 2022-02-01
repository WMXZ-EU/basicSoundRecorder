#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
/* following from https://github.com/sstaub/TeensyID/blob/master/TeensyID.cpp */
#if defined ARDUINO_TEENSY40 || defined ARDUINO_TEENSY41
  #define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
  #define CPU_RESTART_VAL 0x5FA0004
  #define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL)
#else
  #define CPU_RESTART_ADDR
  #define CPU_RESTART_VAL
  #define CPU_RESTART 
#endif

uint32_t getTeensySerial(void);
extern uint32_t SerNum;
#endif
