
    #define FREQS {32000, 44100, 48000, 96000, 192000, 220500, 240000, 360000}
    #define IFR 3

    #define FSAMP 96000
    #define NSAMP 128

    #define NPORT_I2S 1
    #define NCHAN_I2S 2 // I2S channel number

    #define NCHAN_ACQ 2 // aduisition channel number

    #define NDBL (4)

    #define DirPrefix  "D"
    #define FilePrefix "F"

    #define NBUF_I2S (NPORT_I2S*NCHAN_I2S*NSAMP)    
    #define NBUF_ACQ (NCHAN_ACQ*NSAMP)

    #define MIN_SPACE (5*60*(fsamp/512)*NCHAN_ACQ*4/1024)   // min space on disk (sectors)

    
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
