
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

    #define MIN_SPACE (5*60*FSAMP*NCHAN_ACQ*4)        // min space on disk

