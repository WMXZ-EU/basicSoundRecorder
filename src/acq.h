#ifndef ACQ_H
#define ACQ_H
// #define I2S_CONFIG 0 // D11, D9,  D23, D13, D22 (MCLK, TX_BCLK, TX_FS, RX_D0, TX_D0) T3.x
// #define I2S_CONFIG 0 // D23, D21, D20, D8,  D7  (MCLK, RX_BCLK, RX_FS, RX_D0, TX_D0) T4.x
//
// #define I2S_CONFIG 2 // D35,PTC8(4); D36,PTC9(4) D37,PTC10(4); D27,PTA15(6) (MCLK,RX_BCLK,RX_FS,TX_D0)
// #define I2S_CONFIG 3 // D35,PTC8(4); D36,PTC9(4) D37,PTC10(4); D27,PTA15(6) (MCLK,RX_BCLK,RX_FS,TX_D0)
    #define I2S_CONFIG 0

    extern volatile uint32_t acq_count;
    extern volatile uint32_t acq_miss;

    void acq_stopClocks(void);
    void acq_startClocks(void);

    void acq_init(int fsamp);
    void acq_start(void);
    void acq_stop(void);
#endif