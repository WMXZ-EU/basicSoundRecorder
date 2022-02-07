#include <stdint.h>

#include "config.h"
#include "filing.h"
#include "acq.h"
#include "adc.h"

/********************************************************************************/
#include "DMAChannel.h"
static DMAChannel dma;

static void acq_isr(void);

DMAMEM static uint32_t tdm_rx_buffer[2*NBUF_I2S] __attribute__((aligned(32)));

static uint32_t acq_rx_buffer[NBUF_ACQ];
volatile uint32_t acq_count=0;
volatile uint32_t acq_miss=0;


#ifndef I2S_DMA_PRIO
  #define I2S_SAI_PRIO 4*16
  #define I2S_DMA_PRIO 5*16
#endif

#if defined(ARDUINO_TEENSY32) || defined(ARDUINO_TEENSY36)

    #define IMXRT_CACHE_ENABLED 0
//------------------------------------- I2S -------------------------------------

//WMXZ hardware specific defines
//    #if ADC_MODEL == CS5366
//        #define CH6
//        #define I2S_CONFIG 1 // D39, D11, D12, D13 (MCLK,RX_BCLK,RX_FS,RX_D0)
//    #else
//        #define CH8
////        #define I2S_CONFIG 2 // D35,PTC8(4); D36,PTC9(4) D37,PTC10(4); D27,PTA15(6) (MCLK,RX_BCLK,RX_FS,TX_D0)
//    #endif

    #define MCLK_SRC  3
    // set MCLK to 48 MHz
    #define BCLK_DIV 3 //((48MHz/93.75kHz)/(2*64)-1) // 64 = 2*32 bit 

    #define MCKL_SCALE 1
    // SCALE 1: 48000/512 kHz = 93.75 kHz
    #if (F_PLL == 96000000) //PLL is 96 MHz for F_CPU==48 or F_CPU==96 MHz
        #define MCLK_MULT 1 
        #define MCLK_DIV  (2*MCKL_SCALE) 
        //  #define MCLK_DIV  (3*MCKL_SCALE) 
    #elif F_PLL == 120000000
        #define MCLK_MULT 2
        #define MCLK_DIV  (5*MCKL_SCALE)
    #elif F_PLL == 144000000
        #define MCLK_MULT 1
        #define MCLK_DIV  (3*MCKL_SCALE)
    #elif F_PLL == 192000000
        #define MCLK_MULT 1
        #define MCLK_DIV  (4*MCKL_SCALE)
    #elif F_PLL == 240000000
        #define MCLK_MULT 1
        #define MCLK_DIV  (5*MCKL_SCALE)
    #else
        #error "set F_CPU to (48, 96, 120, 144, 192, 240) MHz"
    #endif

    const int32_t fsamp0=(((F_PLL*MCLK_MULT)/MCLK_DIV)/512);
    
    void acq_stopClocks(void)
    {
        SIM_SCGC6 &= ~SIM_SCGC6_DMAMUX;
        SIM_SCGC7 &= ~SIM_SCGC7_DMA;
        SIM_SCGC6 &= ~SIM_SCGC6_I2S;
    }

    void acq_startClocks(void)
    {
    SIM_SCGC6 |= SIM_SCGC6_I2S;
    SIM_SCGC7 |= SIM_SCGC7_DMA;
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
    }

    void acq_start(void)
    {
        acq_startClocks();
        I2S0_RCSR |= (I2S_RCSR_RE | I2S_RCSR_BCE);
    }

    void acq_stop(void)
    {
        I2S0_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);
        acq_stopClocks();
    }

    void mckl_init(int src, int mult, int div)
    {
        SIM_SCGC6 |= SIM_SCGC6_I2S;
        // enable MCLK output
        I2S0_MDR = I2S_MDR_FRACT((mult-1)) | I2S_MDR_DIVIDE((div-1));
        while(I2S0_MCR & I2S_MCR_DUF);
        I2S0_MCR = I2S_MCR_MICS(src) | I2S_MCR_MOE;

    }

    void acq_init(int fsamp)
    {
        Serial.printf("%d %d\n",fsamp,fsamp0);

        acq_startClocks();

        #if I2S_CONFIG==0
            CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
        	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
            CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
            CORE_PIN13_CONFIG = PORT_PCR_MUX(4);  //pin 13, PTC5,  I2S0_RXD0
        #elif I2S_CONFIG==1
            CORE_PIN39_CONFIG = PORT_PCR_MUX(6);  //pin39, PTA17, I2S0_MCLK
            CORE_PIN11_CONFIG = PORT_PCR_MUX(4);  //pin11, PTC6,  I2S0_RX_BCLK
            CORE_PIN12_CONFIG = PORT_PCR_MUX(4);  //pin12, PTC7,  I2S0_RX_FS 
            CORE_PIN13_CONFIG = PORT_PCR_MUX(4);  //pin13, PTC5,  I2S0_RXD0
        #elif I2S_CONFIG==2
            CORE_PIN35_CONFIG = PORT_PCR_MUX(4) | PORT_PCR_SRE | PORT_PCR_DSE;  //pin35, PTC8,   I2S0_MCLK (SLEW rate (SRE)?)
            CORE_PIN36_CONFIG = PORT_PCR_MUX(4);  //pin36, PTC9,   I2S0_RX_BCLK
            CORE_PIN37_CONFIG = PORT_PCR_MUX(4);  //pin37, PTC10,  I2S0_RX_FS
            CORE_PIN27_CONFIG = PORT_PCR_MUX(6);  //pin27, PTA15,  I2S0_RXD0
        #endif
        I2S0_RCSR=0;

        mckl_init(MCLK_SRC, MCLK_MULT, MCLK_DIV);
        
#if 0
        I2S0_RMR=0; // enable receiver mask
        I2S0_RCR1 = I2S_RCR1_RFW(3); 

        I2S0_RCR2 = I2S_RCR2_SYNC(0) 
                    | I2S_RCR2_BCP ;
                    
        I2S0_RCR3 = I2S_RCR3_RCE; // single rx channel

        I2S0_RCR4 = I2S_RCR4_FRSZ((NCHAN_I2S-1)) // 8 words (TDM - mode)
                    | I2S_RCR4_FSE  // frame sync early
                    | I2S_RCR4_MF;
        #if ADC_STEREO
            I2S0_RCR4 |=  I2S_RCR4_SYWD(31);
        #endif
        
        I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);
#else
	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(BCLK_DIV);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(BCLK_DIV);
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

#endif
// configuration of DMA
        dma.TCD->SADDR = &I2S0_RDR0;
        dma.TCD->SOFF = 0;
        dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	    dma.TCD->NBYTES_MLOFFNO = 4;
        dma.TCD->SLAST = 0;
        dma.TCD->DADDR = tdm_rx_buffer;
        dma.TCD->DOFF = 4;
        dma.TCD->CITER_ELINKNO = 2*NBUF_I2S;
        dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
        dma.TCD->BITER_ELINKNO = 2*NBUF_I2S;
        dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
        //
        dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
        NVIC_SET_PRIORITY(DMAMUX_SOURCE_I2S0_RX,I2S_SAI_PRIO);
        //
        I2S0_RCSR =  I2S_RCSR_FRDE | I2S_RCSR_FR;
        dma.attachInterrupt(acq_isr, I2S_DMA_PRIO);	
        //
        dma.enable();        
    }

#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)

#ifndef DMAMUX_DISABLE
    #define DMAMUX_DISABLE		0
    #define DMAMUX_TRIG			64
    #define DMAMUX_ENABLE		128
#endif

    /******************************* 4-chan TDM ***********************************************/
    #define IMXRT_CACHE_ENABLED 2 // 0=disabled, 1=WT, 2= WB

    FLASHMEM
    void set_audioClock(int nfact, int32_t nmult, uint32_t ndiv, bool force=false) // sets PLL4
    {
        if (!force && (CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_ENABLE)) return;

        CCM_ANALOG_PLL_AUDIO = CCM_ANALOG_PLL_AUDIO_BYPASS | CCM_ANALOG_PLL_AUDIO_ENABLE
                    | CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2) // 0: 1/4; 1: 1/2; 2: 1/1
                    | CCM_ANALOG_PLL_AUDIO_DIV_SELECT(nfact);

        CCM_ANALOG_PLL_AUDIO_NUM   = nmult & CCM_ANALOG_PLL_AUDIO_NUM_MASK;
        CCM_ANALOG_PLL_AUDIO_DENOM = ndiv & CCM_ANALOG_PLL_AUDIO_DENOM_MASK;
        CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_POWERDOWN;//Switch on PLL
        while (!(CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK)) {}; //Wait for pll-lock
        
        const int div_post_pll = 1; // other values: 2,4
        CCM_ANALOG_MISC2 &= ~(CCM_ANALOG_MISC2_DIV_MSB | CCM_ANALOG_MISC2_DIV_LSB);
        if(div_post_pll>1) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_LSB;
        if(div_post_pll>3) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_MSB;
        
        CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_BYPASS;   //Disable Bypass
        Serial.printf("PLL %f\r\n",24.0f*((float)nfact+(float)nmult/(float)ndiv));
    }

    void mckl_init(int fsamp)
    {
        CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

        // if either transmitter or receiver is enabled, do nothing
        if (I2S1_RCSR & I2S_RCSR_RE) return;
        //PLL:
        int fs = fsamp;
        int ovr = 2*(NCHAN_I2S*32);

        // PLL between 27*24 = 648MHz und 54*24=1296MHz
        int n0 = 26; // targeted PLL frequency (n0*24 MHz) n0>=27 && n0<54
        int n1, n2;
        do
        {   n0++;
            n1=0;
            do
            {   n1++; 
                n2 = 1 + (24'000'000 * n0) / (fs * ovr * n1);
            } while ((n2>64) && (n1<=8));
        } while ((n2>64 && n0<54));
        Serial.printf("fs=%d, n1=%d, n2=%d, %d (>=27 && <54) ", fs, n1,n2,n1*n2*(fs/1000)*ovr/24000);

        double C = ((double)fs * ovr * n1 * n2) / (24000000.0f);
        Serial.printf(" C=%f\r\n",C);
        int c0 = C;
        int c2 = 10'000;
        int c1 =  C * c2 - (c0 * c2);
        set_audioClock(c0, c1, c2);

        // clear SAI1_CLK register locations
        CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
            | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4

        CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
            | CCM_CS1CDR_SAI1_CLK_PRED((n1-1))  // &0x07  // <8
            | CCM_CS1CDR_SAI1_CLK_PODF((n2-1)); // &0x3f  // <64

        IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
                | (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK
    }

    void acq_init(int fsamp)
    {
        mckl_init(fsamp);

        // configuration of I2S
        I2S1_RMR = 0;
        I2S1_RCR1 = I2S_RCR1_RFW(4);
        I2S1_RCR2 = I2S_RCR2_SYNC(0) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
                    | I2S_RCR2_BCD | I2S_RCR2_DIV(0);


        I2S1_RCR4 = I2S_RCR4_FRSZ((NCHAN_I2S-1)) | I2S_RCR4_MF
                   | I2S_RCR4_FSE | I2S_RCR4_FSD;
        I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

        #if ADC_STEREO
            I2S1_RCR4 |=  I2S_RCR4_SYWD(31);
        #endif

#if I2S_CONFIG==0
    	CORE_PIN23_CONFIG = 3;  //1:MCLK 
        CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
        CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
#endif

        // configuration of DMA
        dma.TCD->SADDR = &I2S1_RDR0;
        dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);

#if NPORT_I2S==1
        I2S1_RCR3 = I2S_RCR3_RCE; // single data port

		CORE_PIN8_CONFIG  = 3;  //1:RX_DATA0
		IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // GPIO_B1_00_ALT3, pg 873

        dma.TCD->SOFF = 0;
	    dma.TCD->NBYTES_MLOFFNO = 4;
        dma.TCD->SLAST = 0;

#elif NPORT_I2S==2
        I2S1_RCR3 = I2S_RCR3_RCE_2CH; // dual data port
        //
		CORE_PIN8_CONFIG = 3;   //RX_DATA0
		CORE_PIN6_CONFIG = 3;   //RX_DATA1
		IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // GPIO_B1_00_ALT3, pg 873
		IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873

        // configuration of DMA
        dma.TCD->SOFF = 4;
	    dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE |
                                   DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8) |
                                   DMA_TCD_NBYTES_MLOFFYES_NBYTES(8);
        dma.TCD->SLAST = -8;
#endif
        dma.TCD->DADDR = tdm_rx_buffer;
        dma.TCD->DOFF = 4;
        dma.TCD->CITER_ELINKNO = 2*NBUF_I2S/NPORT_I2S;
        dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
        dma.TCD->BITER_ELINKNO = 2*NBUF_I2S/NPORT_I2S;
        dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
        //
        dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);
        NVIC_SET_PRIORITY(DMAMUX_SOURCE_SAI1_RX,I2S_SAI_PRIO);
        //
        I2S1_RCSR =  I2S_RCSR_FRDE | I2S_RCSR_FR;
        dma.attachInterrupt(acq_isr, I2S_DMA_PRIO);	
        //
        dma.enable();        
    }

    void acq_start(void)
    {
        I2S1_RCSR |= (I2S_RCSR_RE | I2S_RCSR_BCE);
    }

    void acq_stop(void)
    {
        I2S1_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);
    }
#endif


const int adc_shift=8;
#if ADC_STEREO // have stereo I2S
    void extract(void *out, void *inp)
    {   
        int32_t *dout = (int32_t *) out;
        int32_t *din  = (int32_t *) inp;
        for(int ii=0; ii < NSAMP; ii++)
        {   dout[0+ii*NCHAN_ACQ] = din[0+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
            dout[1+ii*NCHAN_ACQ] = din[1+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
        }
    }
#else // have TMA 8 channels
    void extract(void *out, void *inp)
    {   int32_t *dout = (int32_t *) out;
        int32_t *din  = (int32_t *) inp;
        for(int ii=0; ii < NSAMP; ii++)
        {   dout[0+ii*NCHAN_ACQ] = din[0+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
            dout[1+ii*NCHAN_ACQ] = din[2+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
            dout[2+ii*NCHAN_ACQ] = din[4+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
            dout[3+ii*NCHAN_ACQ] = din[1+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
            dout[4+ii*NCHAN_ACQ] = din[3+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
            dout[5+ii*NCHAN_ACQ] = din[5+ii*NPORT_I2S*NCHAN_I2S]>>adc_shift;
        }
    }
#endif

    void acq_isr(void)
    {
        uint32_t daddr;
        uint32_t *src;

        dma.clearInterrupt();
        asm volatile("dsb");
        daddr = (uint32_t) dma.destinationAddress();

        if (daddr < (uint32_t) &tdm_rx_buffer[NBUF_I2S]) {
            // DMA is receiving to the first half of the buffer
            // need to remove data from the second half
            src = &tdm_rx_buffer[NBUF_I2S];
        } else {
            // DMA is receiving to the second half of the buffer
            // need to remove data from the first half
            src = &tdm_rx_buffer[0];
        }

        #if IMXRT_CACHE_ENABLED >=1
            arm_dcache_delete((void*)src, sizeof(tdm_rx_buffer) / 2);
        #endif

        extract((void *) acq_rx_buffer, (void *) src);
        if(!pushData(acq_rx_buffer)) acq_miss++;
        acq_count++;
    }
