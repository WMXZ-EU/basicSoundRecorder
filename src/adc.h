#ifndef ADC_H
#define ADC_H

#define SGTL5000 0
#define CS5381 1
#define CS5361 2
#define CS5366 3
#define CS5368 4
#define TLDV320ADC6140 5     
#define TLDV320ADC6140_2 6   // (dual ADC)

#define ADC_MODEL SGTL5000

#define ADC_STEREO ((ADC_MODEL == SGTL5000) || (ADC_MODEL == CS5381) || (ADC_MODEL == CS5361))

    void adc_init(void);
    void setAGain(int8_t again);
    void adcStatus(void) ;
    bool adc_enable(const unsigned extMCLK, const uint32_t pllFreq = (4096.0l * 44100l));

#endif
