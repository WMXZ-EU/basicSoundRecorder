
    #define FSAMP 96000
    #define NSAMP 128

    #define NPORT_I2S 1
    #define NCHAN_I2S 2
    #define NBUF_I2S (NPORT_I2S*NCHAN_I2S*NSAMP)  // I2S channel number

    #define NCHAN_ACQ 2                           // aduisition channel number
    #define NBUF_ACQ (NCHAN_ACQ*NSAMP)

    #define NDBL (4)

