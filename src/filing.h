#ifndef FILING_H
#define FILING_H


    #include <stdint.h>
    #include "SD.h"
    #include "SdFat.h"
    #include "SPI.h"

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

    #if USE_LONG_FILE_NAMES != 1
        #error "In SdFatConfig.h set USE_LONG_FILE_NAMES to 1"
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
