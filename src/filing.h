#ifndef FILING_H
#define FILING_H

    #include <stdint.h>
    #include "SD.h"
    #include "SdFat.h"
    #include "SPI.h"

    /**
     * @brief Definitions
     * 
     */
    #define FSAMP 48000
    #define NSAMP 128

    #define NPORT_I2S 1
    #define NCHAN_I2S 2
    #define NBUF_I2S (NPORT_I2S*NCHAN_I2S*NSAMP)  // I2S channel number

    #define NCHAN_ACQ 2                           // aduisition channel number
    #define NBUF_ACQ (NCHAN_ACQ*NSAMP)

    #define NDBL (4)
    #define MAX_DISK_BUFFER (NDBL*NBUF_ACQ)

    #define MAXBUF (200)
    #define MIN_SPACE (5*60*MAX_DISK_BUFFER) 

    #define DirPrefix  "D"
    #define FilePrefix "F"

    /**
     * @brief Constants
     * 
     */
    #if defined(ARDUINO_TEENSY36)
        const int teensy=36;
    #elif defined(ARDUINO_TEENSY40)
        const int teensy=40;
    #elif defined(ARDUINO_TEENSY41)
        const int teensy=41;
    #endif

    //-----------------------------------------------------------------------------------------------------
    // interface to circular data buffer
    uint16_t getCount ();
    uint16_t pushData(uint32_t * src);
    uint16_t pullData(uint32_t * dst, uint32_t ndbl);

    /*
    *  in SdFatConfig.h
    *  
    *  #define USE_FAT_FILE_FLAG_CONTIGUOUS 1
    *  #define SDFAT_FILE_TYPE 3
    *  #define SPI_DRIVER_SELECT 3
    *  #define SD_CHIP_SELECT_MODE 1
    *  #define USE_LONG_FILE_NAMES 1
    *  #define MAINTAIN_FREE_CLUSTER_COUNT 1
    *
    */
    /* check proper SdFatConfig defines */
    #if USE_FAT_FILE_FLAG_CONTIGUOUS != 1
        #error "In SdFatConfig.h set USE_FAT_FILE_FLAG_CONTIGUOUS to 1"
    #endif

    #if SDFAT_FILE_TYPE != 3
        #error "In SdFatConfig.h set SDFAT_FILE_TYPE to 3"
    #endif

    #if SPI_DRIVER_SELECT != 3
        #error "In SdFatConfig.h set SPI_DRIVER_SELECT to 3"
    #endif

    #if SD_CHIP_SELECT_MODE != 1
        #error "In SdFatConfig.h set D_CHIP_SELECT_MODE to 1"
    #endif

    #if   MAINTAIN_FREE_CLUSTER_COUNT != 1
        #error "In SdFatConfig.h set MAINTAIN_FREE_CLUSTER_COUNT to 1"
    #endif

    #if MAINTAIN_FREE_CLUSTER_COUNT != 1
        #error "In SdFatConfig.h set MAINTAIN_FREE_CLUSTER_COUNT to 1"
    #endif

    enum STATUS
    {
        MUST_REBOOT=-2,
        //
        STOPPED=-1,
        CLOSED=0,
        OPENED=1,
        RUNNING=2,
        DOCLOSE=3,
        MUSTSTOP=4
    };

    extern int t_acq;

    void storage_configure();

    void listDisks(void);
    int16_t checkReboot(void);

    /* main data filing routine */
    int16_t saveData(int16_t status);

#endif
