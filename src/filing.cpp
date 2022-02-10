/**
 * @file filing.cpp
 * @author  Walter Zimmer 
 * @version 1.0
 *
 * @section LICENSE
 *
 * MIT
 * 
 * @section DESCRIPTION
 *
 * contains filing interface
 */
    #include "config.h"
    #include "mTime.h"
    #include "filing.h"
    #include "utils.h"
    #include "adc.h"

     /**
     * @brief Definitions
     * 
     */

    #if defined(ARDUINO_TEENSY36)
        #define MAXBUF (160)
    #elif defined(ARDUINO_TEENSY40)
        #define MAXBUF (200)
    #elif defined(ARDUINO_TEENSY41)
        #define MAXBUF (200)
    #endif
    


    /**
     * @brief Circular Data Buffer
     * 
     */    
    #if defined(ARDUINO_TEENSY41) && defined(USE_EXT_MEM)
        EXTMEM uint32_t data_buffer[MAXBUF*NBUF_ACQ];
    #else
        uint32_t data_buffer[MAXBUF*NBUF_ACQ];
    #endif

    #define MAX_DISK_BUFFER (NDBL*NBUF_ACQ)

    /**
     * @brief Data storage class
     * 
     */
    class Data
    {
        public:
            Data(uint32_t * buffer) 
            { /**
             * @brief Constructor
             * @param buffer is pointer to data store
             * 
             */
                data_buffer=buffer; front_=rear_=0;
            }

            uint16_t push(uint32_t * src)
            { 
                /** 
                 * @brief push data to storage
                 * @param src is pointer to data block
                 */
                uint16_t f =front_ + 1;
                if(f >= MAXBUF) f=0;
                if(f == rear_) return 0;

                uint32_t *ptr= data_buffer+f*NBUF_ACQ;
                memcpy(ptr,src,NBUF_ACQ*4);
                front_ = f;
                return 1;
            }

            uint16_t pull(uint32_t * dst, uint32_t ndbl)
            {   
                /** 
                 * @brief pull data from storage
                 * @param dst is pointer to data blocks
                 * @param ndbl is number of data blocks
                 */
                uint16_t r = (rear_/ndbl) ;
                if(r == (front_/ndbl)) return 0;

                uint32_t *ptr= data_buffer + r*ndbl*NBUF_ACQ;
                memcpy(dst,ptr,ndbl*NBUF_ACQ*4);
                if(++r >= (MAXBUF/ndbl)) r=0;
                rear_ = r*ndbl;
                return 1;
            }
            uint16_t getCount () 
            {  
                /**
                 * @brief get number of data blocks in storage
                 * 
                 */
                if(front_ >= rear_) return front_ - rear_; return front_+ MAXBUF -rear_; 
            }

    private:    
        uint16_t front_, rear_;
        uint32_t *data_buffer;

    };

    Data rawData(data_buffer);

    uint16_t getCount () { return rawData.getCount(); }
    uint16_t pushData(uint32_t * src){ return rawData.push(src);}
    uint16_t pullData(uint32_t * dst, uint32_t ndbl) {return rawData.pull(dst,ndbl);}

/****************************** Filing Utility *******************************************/

extern int t_acq;

File file=NULL; // is used by saveData and saveNAD
uint32_t disk_count=0;
uint32_t diskBuffer[MAX_DISK_BUFFER];

SDClass *msd;  // storage of next free disk

static int16_t makeHeader(char *header)
{
    /**
     * @brief Make file header
     * @param header is pointer to header
     * 
     */
    #define MAGIC "WMXZ"
    tmElements_t tm;
    breakTime(rtc_get(), tm);

    int nd=sprintf(header,"%s%04d%02d%02d_%02d%02d%02d",
            MAGIC,tmYearToCalendar(tm.Year),tm.Month,tm.Day,tm.Hour,tm.Minute,tm.Second);
    char *ptr = header+(nd+1);

    int32_t *iptr = (int32_t *) ptr;
    iptr[0] = 4;                    // SW version
    iptr[1] = (int32_t)SerNum;      // serial number
    iptr[2] = fsamp;
    iptr[3] = NCHAN_ACQ;
    iptr[4] = t_acq;
    iptr[5] = 0;

    uint32_t *uptr = (uint32_t*) header;
    uptr[127] = 0x55555555;
    //
    return 1;
}

static uint32_t dummy_buffer[MAX_DISK_BUFFER];
/******************************** SPI Interface *****************************************/
// following is from SdFat examples to allow transactions to be avoided between open and close
//
// This is a simple driver based on the the standard SPI.h library.
// You can write a driver entirely independent of SPI.h.
// It can be optimized for your board or a different SPI port can be used.
// The driver must be derived from SdSpiBaseClass.
// See: SdFat/src/SpiDriver/SdSpiBaseClass.h
class MySpiClass : public SdSpiBaseClass {
     /**
     * @brief SPI interface class
     * 
     */

    public:
        // Initialize the SPI bus.
        void begin(SdSpiConfig config) {  (void) config; SPI.begin(); }

        // Activate SPI hardware with correct speed and mode.
        void activate() { if(doTransactions) SPI.beginTransaction(m_spiSettings); }
        // Deactivate SPI hardware.
        void deactivate() { if(doTransactions) SPI.endTransaction(); }

        // Receive a byte.
        uint8_t receive() { return SPI.transfer(0XFF); }

        // Send a byte.
        void send(uint8_t data) { SPI.transfer(data); }

        // Receive multiple bytes.  
        // Replace this function if your board has multiple byte receive.
        uint8_t receive(uint8_t* buf, size_t count) 
        { memset(buf, 0XFF, count); SPI.transfer(buf, count); return 0; }

        // Send multiple bytes.
        // Replace this function if your board has multiple byte send.
        void send(const uint8_t* buf, size_t count) 
        {  SPI.transfer((void *)buf, (void *) dummy_buffer, count); }

        // Save SPISettings for new max SCK frequency
        void setSckSpeed(uint32_t maxSck) {  m_spiSettings = SPISettings(maxSck, MSBFIRST, SPI_MODE0); }

    private:
        SPISettings m_spiSettings;

    public:
        bool doTransactions=true;

};
MySpiClass mySpi;

void doTransactions(bool val) {mySpi.doTransactions=val;}

void sdCsInit(SdCsPin_t pin) { pinMode(pin, OUTPUT); digitalWriteFast(pin, HIGH);}
void sdCsWrite(SdCsPin_t pin, bool level) { digitalWriteFast(pin, level); }

#define SPI_SPEED SD_SCK_MHZ(33)  // adjust to sd card 
#if USE_SDIO==2
    #define SD_CONFIG SdioConfig(FIFO_SDIO)
    #define SD_MOSI 11
    #define SD_MISO 12
    #define SD_SCK  13
#endif
#if defined(ARDUINO_TEENSY36)
    #define SD_MOSI 7
    #define SD_MISO 12
    #define SD_SCK  14
#endif

//    const char *sd_str[]={"sdio",   "sd1","sd2","sd3","sd4","sd5","sd6"};
//    const int cs[] = {BUILTIN_SDCARD,  34,   33,   35,   36,   37,  38 };

    const char *sd_str[]={"sd1"};
    const int cs[] = {10 };

    const int nsd = sizeof(cs)/sizeof(int);

    SDClass sdx[nsd];
    uint32_t diskSize[nsd];
    uint32_t diskSpace[nsd];
    uint32_t clusterSize[nsd];

// Call back for file timestamps.  Only called for file create and sync(). needed by SDFat-beta
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) 
{       
    tmElements_t tm;
    breakTime(rtc_get(), tm);

    *date = FS_DATE(tmYearToCalendar(tm.Year),tm.Month, tm.Day);
    *time = FS_TIME(tm.Hour, tm.Minute, tm.Second);
    *ms10 = tm.Second & 1 ? 100 : 0;
}

void storage_configure()
{
    #if defined SD_SCK
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    // Set Time callback
    FsDateTime::callback = dateTime;

    for(int ii=0; ii<nsd; ii++)
    { Serial.println(ii);

      uint16_t status=0;
      if(cs[ii] == BUILTIN_SDCARD)
      {
            uint16_t tries=0;
            while(!sdx[ii].sdfs.begin(SdioConfig(FIFO_SDIO)) && tries<10)
            { Serial.print(tries); Serial.print(" "); tries++; delay(1000); } 

            if(tries<10) 
            {
                status=1;
            }
            else
            {
                Serial.println("No sdio storage"); 
            }
      }
      else if(cs[ii]<BUILTIN_SDCARD)
      { 
            sdCsInit(cs[ii]);
            delay(100);
            uint16_t tries=0;
            while(!sdx[ii].sdfs.begin(SdSpiConfig(cs[ii], SHARED_SPI, SPI_SPEED, &mySpi)) && tries<10)
            { Serial.print(tries); Serial.print(" "); tries++; delay(1000); } 

            if(tries<10) 
            {
                status=1;
            }
            else
            {
                Serial.println("No spi storage");
            }
      }
      if (status)
      {
            uint64_t totalSize = sdx[ii].totalSize();
            uint64_t usedSize  = sdx[ii].usedSize();

            Serial.printf("Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
            Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);

            Serial.print(sdx[ii].sdfs.clusterCount()); Serial.print(" "); 
            Serial.print(sdx[ii].sdfs.freeClusterCount()); Serial.print(" ");

            clusterSize[ii] = sdx[ii].sdfs.bytesPerCluster()/512; // count sectors
            Serial.println(clusterSize[ii]) ;Serial.print(" ");

            diskSpace[ii]=diskSize[ii]=sdx[ii].sdfs.freeClusterCount();
            Serial.println(diskSize[ii]) ;Serial.print(" ");
      }
      else
      {
            diskSize[ii]=0;
            diskSpace[ii]=0;
            clusterSize[ii]=1;
      }
    }
}

void listDisks(void)
{
    for(int ii=0;ii<nsd;ii++)
    {
        Serial.print("\n"); 
        Serial.printf("Storage %d %d %s ",ii,cs[ii],sd_str[ii]);
        Serial.printf("%d %d",sdx[ii].sdfs.freeClusterCount(),diskSize[ii]);
    }
    Serial.print("\n"); 
}

int16_t checkReboot(void)
{
    int ii=0;
    while((ii<nsd) && (diskSpace[ii]<MIN_SPACE)) ii++;
    if(ii<nsd) CPU_RESTART;
    return -1;
}

static int isd=(nsd>1)?1:0; // fist SPI disk is #1 if there are more than one
extern int32_t hour_; // to force newHour on new disk

static SDClass *checkDiskSpace(void)
{   static int isd_ = nsd;

    doTransactions(true);
    while((isd<nsd) && ((diskSize[isd]==0) 
            || ((diskSpace[isd]=sdx[isd].sdfs.freeClusterCount()) < MIN_SPACE) )) isd++;
    doTransactions(false);
    if(isd<nsd) 
    {   sdx[isd].sdfs.chvol();
        Serial.println(); Serial.print(isd);
        Serial.print(": "); Serial.print(sdx[isd].sdfs.freeClusterCount());
    }
    //
    if(isd != isd_) hour_=0; // set hour_ to zero to trigger creation of directories in new disk
    isd_=isd;
    if(isd==nsd) return (SDClass *) 0; else return &sdx[isd];
}

static int16_t newDirectory(char *dirName, int dirFlag)
{ if(newHour())
    {   
        tmElements_t tm;
        breakTime(rtc_get(), tm);
        if(!dirFlag)
        {
            sprintf(dirName, "/%s%06x_%04d%02d%02d/%02d/", 
                          DirPrefix,(unsigned int)SerNum,
                                  tmYearToCalendar(tm.Year),tm.Month, tm.Day, tm.Hour);
        }
        else
        {
            sprintf(dirName, "/%s%06x_%04d%02d%02d/%02d_%02d/", 
                          DirPrefix,(unsigned int)SerNum,
                                  tmYearToCalendar(tm.Year),tm.Month, tm.Day, tm.Hour,dirFlag);
        }

        //
        Serial.print("\n"); Serial.print(dirName);
        #if PRINT_LOGFILE>0
        log_start();
        if(logFile) {logFile.print("\n");logFile.print(dirName);}
        #endif
        return 1;
    }
    return 0;

}
static int16_t newFileName(char *fileName)
{
    tmElements_t tm;
    breakTime(rtc_get(), tm);
	sprintf(fileName, "%s_%02d%02d%02d.bin", FilePrefix, tm.Hour, tm.Minute, tm.Second);
    //
    Serial.print("\n"); Serial.print(isd); Serial.print(": ");Serial.print(fileName);
    #if PRINT_LOGFILE>0
        if(logFile) 
        {   logFile.print("\n"); logFile.print(isd); logFile.print(": "); logFile.print(fileName);}
    #endif
    return 1;
}

/* main data filing routine */
int16_t saveData(int16_t status)
{   static char dirName[80];
    static char fileName[80];
    static char header[512];
    static int dirFlag=0;

    if(status<CLOSED) return status; // we are stopped: don't do anything

    //fetch data from circular buffer
    if(pullData(diskBuffer,NDBL))
    {   disk_count++;
        if(status==CLOSED) // file closed: should open
        {   //doTransactions(true);
            if(!(msd=checkDiskSpace())) return MUST_REBOOT;
            //
            if(newDirectory(dirName,dirFlag)) 
            {   if(!msd->sdfs.exists(dirName) && !msd->sdfs.mkdir(dirName)) dirFlag++;          
                if(!msd->sdfs.chdir(dirName)) dirFlag++; else dirFlag=0;
            }
            //
            if(dirFlag>5) return MUST_REBOOT;   // too many directory errors
            if(dirFlag>0) return CLOSED;        // create new directory with different name

            if(newFileName(fileName))
            {   
                file = msd->open(fileName, FILE_WRITE_BEGIN); 
                if(file) 
                    status = OPENED; 
                else 
                {   Serial.println("Failing open file");
                    return MUST_REBOOT; 
                }
            } 
            else
            {
               return MUST_REBOOT; // if file open fails: don't do anything
            }
        }
        //
        if(status==OPENED) // file is open: write first record (header)
        {   makeHeader(header);
            if(file.write((const void*)header,512) < 512) return MUST_REBOOT; else status=2;
        }
        //
        if(status>=RUNNING) // file is open, header written: store data records
        {   
            if(file.write((const void *)diskBuffer,4*MAX_DISK_BUFFER) < 4*MAX_DISK_BUFFER) return MUST_REBOOT;
        }
    }
    // following is done independent of data availability
    if(status==DOCLOSE) // should close file
    {
        // writes are done, so enable again transaction activations
        file.flush();
        file.close();
        status = CLOSED;
    }
    if(status==MUSTSTOP) // should close file and stop
    {   
        file.flush();
        file.close();
        status = STOPPED;
    }
    return status;
}
