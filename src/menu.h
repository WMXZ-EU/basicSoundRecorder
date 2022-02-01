#ifndef MENU_H
#define MENU_H
/********************************* Menu ***********************************************/
#include <stdint.h>

uint32_t get_Lock(void);
void set_Lock(uint32_t t_start);

void storeConfig(uint16_t *store, int ns);
void loadConfig(uint16_t *store, int ns);
void saveParameters(void);
void loadParameters(void);

void listDisks(void);
void resetMTP(void) ;
void dumpMTP(void) ;

void setAGain(int8_t again);

int16_t menu(void);

#endif
