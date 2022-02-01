#ifndef CONFIG_H
#define CONFIG_H

  #define GLIDER 1  // is T3.6 based cVAS for glider
  #define FLOAT 2   // is T3.6 based cVAS for float
  #define OOF 3     // is T4.1 based cVAS for OOF buoys

  #if defined(ARDUINO_TEENSY41)
    const int teensy=41;
    #define PROJECT OOF
  #elif defined(ARDUINO_TEENSY36)
    const int teensy=36;
    #define PROJECT FLOAT
  #endif

  #if defined(USB_MTPDISK) || defined(USB_MTPDISK_SERIAL)
    #define USE_MTP 1
  #else
    #define USE_MTP 0
  #endif

  #include "SD.h"
  #include "SdFat.h"
  #if USE_MTP==1
    #include "MTP.h"
  #endif

  #define USE_EXT_MEM
  #if defined(ARDUINO_TEENSY41) && defined(USE_EXT_MEM)
      #ifndef EXTMEM
      #define EXTMEM __attribute__((section(".externalram"))) // in avr/pgmspace.h
      #endif
      #define MAXBUF ((2*2500/NDBL)*NDBL) // < 5461 
  #else
      #define MAXBUF ((40/NDBL)*NDBL)
  #endif

/********************** Defines **********************************/
#define TEST         1  // general testing (0: production run, 1: limit nb , 2: no nad)
#define ATEST        0  // analog gain test (0: production run, 1: gain test, 2: insert signal)
#define PFILT        1  // processing filter 0: no, 1: diference
#define MUST_WAIT    0  // always wait beginning program for usb serial independent of START_MODE (0 for nowait)
#define START_MODE  -1  // put to -1 if startup goes to stopped mode or 0 to start acq immediately
#define P_SIM      1.0f // simulate "under pressure", set to 0.0f if production

#define FSAMP 96000
#define NSAMP 128


#if PROJECT==FLOAT
  #define NCHAN_I2S 8
  #define NPORT_I2S 1
#elif PROJECT==OOF
  #define NCHAN_I2S 4
  #define NPORT_I2S 2
#endif
#define NBUF_I2S (NPORT_I2S*NCHAN_I2S*NSAMP)  // I2S channel number

#define NCHAN_ACQ 6                           // aduisition channel number
#define NBUF_ACQ (NCHAN_ACQ*NSAMP)

#define NDBL 8
#define MAX_DISK_BUFFER (NDBL*NBUF_ACQ)

#if PROJECT==FLOAT
//  #define DOCOMPRESS 0  // 0 no compression; 1 integer compression; 2 integer+spectrum; 3 integer+intensity
//  #define SHIFT 8  // is 8 + digital shift
  #define DOCOMPRESS 1  // 0 no compression; 1 integer compression; 2 integer+spectrum; 3 integer+intensity
  #define SHIFT 12  // is 8 + digital shift
#elif PROJECT==OOF
  #define DOCOMPRESS 1  // 0 no compression; 1 integer compression; 2 integer+spectrum; 3 integer+intensity
  #define SHIFT 14  // is 8 + digital shift
#endif

#if DOCOMPRESS==2
  #define K_MAX (24)   // 8 = (6144/768) minimal ratio for equal sized storage for spectrum average
#elif DOCOMPRESS==3
  #define K_MAX (6)   // 8 = (6144/768) minimal ratio for equal sized storage for spectrum average
#else
  #define K_MAX (8)   // 8 = (6144/768) minimal ratio for equal sized storage for spectrum average
#endif

#define THRESH 3
#define GAIN 0;   // will be used for analog amplifier in ADC

#define PRINT_LOGFILE 0
#define DISP_LOG 1
#if DOCOMPRESS==0
  #define PRINT_DATA 1
  #define PRINT_STAT 0
#else
  #define PRINT_DATA 0
  #define PRINT_STAT 1
#endif
#define PROC_BIT_MIN  6.0f
#define PROC_BIT_MAX 10.0f


#define T_START 0                // start time flag
const int ST[]={2021,05,07,12};  // start time {YYYY, MM,DD, HH } // edit to point into the future

#define TACQ 60             // file size in seconds
#define TON  (5*60)         // acquisition on time in seconds 
#define TOFF (0)            // acquisition off time in seconds

// max number of buffers (for 1 min files): 60*96000/(8*128) = 5625
// following guard space should cover at least 3 files
//#define DISKGUARD (200'000) // how many disk blocks must be on disk to write a file (max fileSize/max_diskbuffer)
// amount of disk sectors to keep free 
#define MIN_SPACE 20000 // corresponds to 10 GB on 512 kB clusters

#define CS5366 0
#define CS5368 1
#define TLDV320ADC6140 2     
#define TLDV320ADC6140_2 3   // (dual ADC)

#if PROJECT==FLOAT
  //#define ADC_MODEL CS5366
  #define ADC_MODEL CS5368
  #define DirPrefix  "D"
  #define FilePrefix "F"
  #define HAVE_PRESSURE 0
#elif PROJECT==OOF
  #define ADC_MODEL TLDV320ADC6140_2
  #define DirPrefix  "OOF_"
  #define FilePrefix "F"
  #define HAVE_PRESSURE 1
#endif

#if ADC_MODEL==CS5368
    #define SINGLE_SPEED 0  // < 50 kHz
    #define DOUBLE_SPEED 1  // 50 - 100 kHz
    #define QUAD_SPEED   2  // 100 - 200 kHz

    #if 1
        #define CS_SPEED DOUBLE_SPEED
        #define MCKL_SCALE 1 // 1: for MCLK = 48 MHz (2: MCLK = 24 MHz (only for FS<100kHz))
        // set MCLK to 48 MHz
        // SCALE 1: 48000/512 kHz = 93.75 kHz
    #else
        // 
        #define CS_SPEED QUAD_SPEED
        #define MCKL_SCALE 2 // 1: for MCLK = 48 MHz (2: MCLK = 24 MHz (only for FS<100kHz))
        // set MCLK to 48 MHz
        // SCALE 1: 48000/512 kHz = 93.75 kHz
        // 
    #endif
#endif

#if HAVE_PRESSURE==1
  #define DO_PRESSURE 1
  #define P_START 1.0f        // 1.0f is 10 m depth
  extern float P0;
#endif

#if (ADC_MODEL == CS5366)
  #define I2S_CONFIG  1 // D39, D11, D12, D13 (MCLK,RX_BCLK,RX_FS,TX_D0) // ADC: Wire: 16,17
  #define IMU_VERSION 1 // NAD: Wire: 33,34
#elif (ADC_MODEL == CS5368)
  // 8ch_nanoPAM_1905 (DSpark)
  #define I2S_CONFIG  2 // ADC: T3.6 D35,PTC8(4); D36,PTC9(4) D37,PTC10(4); D27,PTA15(6) (MCLK,RX_BCLK,RX_FS,TX_D0) Wire: SCL16,SDA17
  #define IMU_VERSION 2 // NAD: T3.6 Wire2: SCL3,SDA4
#elif (ADC_MODEL == TLDV320ADC6140)
  #define I2S_CONFIG  2 // ADC: 
  #define IMU_VERSION 3 // NAD: Wire: 18,19
#elif (ADC_MODEL == TLDV320ADC6140_2)
  // cVAS_V3 (KICAD)
  #define I2S_CONFIG  3 // ADC: T4.1 Wire1: SCL16,SDA17
  #define IMU_VERSION 3 // NAD: T4.1 Wire:  SCL19,SDA18  
  //Press: T4.1 Wire2: SCL24,SDA25
#endif

#endif
