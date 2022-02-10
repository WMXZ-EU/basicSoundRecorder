
/********************************* Menu ***********************************************/
#include "config.h"
#include "filing.h"
#include "utils.h"
#include "mTime.h"
#include "menu.h"

#define CONFIG_FILE "/Config.txt"
#define LOCK_FILE "/cVAS.lock"
extern SDClass sdx[];
uint16_t store[16];

extern  int t_acq;

void storeConfig(uint16_t *store, int ns)
{ 
  //  eeprom_write_block(store, 0, ns*sizeof(store[0]));  

    File configFile;
    char text[32];
    configFile=sdx[0].open(CONFIG_FILE,FILE_WRITE_BEGIN);
      for(int ii=0; ii<ns; ii++)
      { sprintf(text,"%10d\r\n",(int) store[ii]); configFile.write((uint8_t*)text,strlen(text)); }
    configFile.close();
  
}


void loadConfig(uint16_t *store, int ns)
{
  //  eeprom_read_block(store, 0, ns*sizeof(store[0]));  

    File configFile;
    char text[32];
    if(!(configFile=sdx[0].open(CONFIG_FILE,FILE_READ))) { store[0]=0; return; } 
    // read from the file until there's nothing else in it:

    for(int ii=0; ii<ns; ii++)
    {   if(configFile.readBytesUntil('\n',text,32)<=0) { store[0]=0; return;} 
        int x; sscanf(text, "%d",&x);
        store[ii]=(int16_t) x;
    }
    configFile.close();
}

void saveParameters(void)
{
  store[0]= 1;
  store[2]= t_acq;

  storeConfig(store, 16);
}

void loadParameters(void)
{
  loadConfig(store,16);
  if(store[0]==1)
  {
    t_acq=store[2];
  }
}


void printMenu(void)
{
  Serial.println("\n Menu");
  Serial.println(" ':h'  : print help");
  Serial.println(" ':s'  : start acquisition");
  Serial.println(" ':e'  : stop acquisition");
  Serial.println(" ':w'  : write parameters to microSD card");
  Serial.println(" ':l'  : list disks");
  Serial.println(" ':r'  : reset MTP");
  Serial.println(" ':b'  : reboot CPU");
//  Serial.println(" ':d'  : dump Index List (testing)");
  Serial.println();
  Serial.println(" '?p'  : show all parameters");
  Serial.println(" '?d'  : get date");
  Serial.println(" '?t'  : get time");
  Serial.println(" '?a'  : get file duration (s)");
  Serial.println();
  Serial.println(" '!d yyyy/mm/dd<cr>'  : set date");
  Serial.println(" '!t hh:mm:ss<cr>'    : set time");
  Serial.println(" '!a val<cr>'  : set file duration (s)");
  Serial.println();
}

void printMenuEntries(void)
{
    tmElements_t tm;
    breakTime(rtc_get(), tm);

  Serial.println("CVAS_V3 Version: " __DATE__  " " __TIME__ );
  Serial.printf("Teensy: %d: %06x\n",teensy,SerNum);
  Serial.printf("Date d = %04d/%02d/%02d\n",tmYearToCalendar(tm.Year), tm.Month, tm.Day); 
  Serial.printf("Time t = %02d:%02d:%02d\n",tm.Hour, tm.Minute, tm.Second); 
  Serial.printf("T_acq a = %d\n",t_acq); 
}

int menuGetInt(int *val)
{ char buffer[40];
  while(!Serial.available());
  int count = Serial.readBytesUntil('\r',buffer,40);
  buffer[count]=0;
//  Serial.println(buffer);
  return sscanf(buffer,"%d",val);
}

int menuGet3Int(int *val1, int *val2, int *val3)
{ char buffer[40];
  while(!Serial.available());
  int count = Serial.readBytesUntil('\r',buffer,40);
  buffer[count]=0;
//  Serial.println(buffer);
  char c1,c2;
  return sscanf(buffer,"%d%c%d%c%d",val1,&c1,val2,&c2,val3);
}

void listDisks(void);
void resetMTP(void) ;
void dumpMTP(void) ;

#if defined (KINETISK) 
#define CPU_RESTART 
#endif

int16_t menu(void)
{
  if(!Serial.available()) return 0;

  char ch=Serial.read();
  if(ch==':')
  { while(!Serial.available()) ; ch=Serial.read();

    if(ch=='s') { Serial.print("\n"); Serial.print("start"); return  +1;} // start acq
    else if(ch=='e') { Serial.print("\n"); Serial.print("stop");  return  -1;} // end acq
    else if(ch=='h') { printMenu(); return 0;} 
    else if(ch=='w') { saveParameters(); return 0;} 
    else if(ch=='l') { listDisks(); return 0;} 
    else if(ch=='b') { Serial.print("\n"); Serial.print("rebooting CPU"); Serial.flush(); delay(100); CPU_RESTART; return 0;} 
  }
  else if(ch=='?') // get info
  {
    while(!Serial.available()) ; ch=Serial.read();

    tmElements_t tm;
    breakTime(rtc_get(), tm);

    if(ch=='p') { printMenuEntries(); return 0;} 
    else if(ch=='d') { Serial.printf("%04d/%02d/%02d\n",tmYearToCalendar(tm.Year), tm.Month, tm.Day); return  0;} // display date
    else if(ch=='t') { Serial.printf("%02d:%02d:%02d\n",tm.Hour, tm.Minute, tm.Second); return  0;} // display time
    else if(ch=='a') { Serial.printf("%d\n",t_acq); return  0;} // file size
  }
  else if(ch=='!') // set 
  { 
    while(!Serial.available()) ;
    ch=Serial.read();
    if(ch=='d') // set date
    { int year,month,day;
      menuGet3Int(&year,&month,&day);
      tmElements_t tm;
      breakTime(rtc_get(), tm);

      setRTCTime(tm.Hour, tm.Minute, tm.Second, day, month, year);
      return  0;
    } 
    else if(ch=='t') // set time
    { int hour,minutes,seconds;
      menuGet3Int(&hour,&minutes,&seconds);

      tmElements_t tm;
      breakTime(rtc_get(), tm);
      setRTCTime(hour, minutes, seconds, tm.Day, tm.Month, tmYearToCalendar(tm.Year));
      return  0;
    } 
    else if(ch=='a') { menuGetInt(&t_acq); return  0;} // file size
  }
  return 0;
}
