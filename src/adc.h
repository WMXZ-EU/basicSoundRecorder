#ifndef ADC_H
#define ADC_H

#define CS5381 0
#define CS5361 1
#define CS5366 2
#define CS5368 3
#define TLDV320ADC6140 44     
#define TLDV320ADC6140_2 5   // (dual ADC)

#define ADC_MODEL CS5381

#define ADC_STEREO ((ADC_MODEL == CS5381) || (ADC_MODEL == CS5361))

    void adc_init(void);
    void setAGain(int8_t again);
    void adcStatus(void) ;
    
#endif
