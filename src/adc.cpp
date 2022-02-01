#include <stdint.h>

#include "adc.h"
#include "i2c.h"

#define CS5361 0
#define CS5366 1
#define CS5368 2
#define TLDV320ADC6140 3     
#define TLDV320ADC6140_2 4   // (dual ADC)

#define ADC_MODEL CS5361

int gain=0;

#if (ADC_MODEL == TLDV320ADC6140) || (ADC_MODEL == TLDV320ADC6140_2)
    const  uint8_t chanMask[2] = {0b1110<<4, 0b1110<<4};
    const  uint8_t chmap[2][4]={{0,1,2,3},{0,1,2,3}};
    int8_t again = gain ;           // 0:42
    const  int8_t dgain = -(int8_t)gain*2;    //(-40.0f*2); // -100:0.5:27
    const int AG[2][4]={{0,0,8,16},{0,24,32,40}};
#endif

#if ADC_MODEL == CS5361 || ADC_MODEL == CS5366 || ADC_MODEL == CS5368
/******************************* CS5366 ********************************************/
    #define CS536X_I2C_ADDR (0x4C)

    #define CS536X_REVI 0x00
    #define CS536X_GCTL 0x01
    #define CS536X_GCTL_CP_EN       ((1 << 7))
    // 0x00 (Default) // Control Port mode
    #define CS536X_GCTL_CLKMODE     ((1 << 6))
    // 0x00 (Default) // 256X 
    #define CS536X_GCTL_MDIV(n)     ((((n)&3) << 4))
    // 0x00 (Default) // divide by 1
    #define CS536X_GCTL_DIF(n)      ((((n)&3) << 2))
    //0x01 I2S format, 0x02 is TDM
    #define CS536X_GCTL_MODE(n)     ((((n)&3) << 0))
    // 0x03 Slave Mode all speeds
    #define CS536X_nOVFL 0x02
    #define CS536X_OVFM 0x03
    #define CS536X_nHPF 0x04
    #define CS536X_PDN 0x06
    #define CS536X_MUTE 0x08
    #define CS536X_nSDOUT 0x0A

    #define TDM_MODE 2
    #define CLK_MDIV 3 //divide by 4

    #define nRST 20 
    //
    #define SINGLE_SPEED 0  // < 50 kHz
    #define DOUBLE_SPEED 1  // 50 - 100 kHz
    #define QUAD_SPEED   2  // 100 - 200 kHz

    #if 1
        #define CS_SPEED DOUBLE_SPEED
        #define MCKL_SCALE 1 // 1: for MCLK = 48 MHz (2: MCLK = 24 MHz (only for FS<100kHz))
        // set MCLK to 48 MHz
        // SCALE 1: 48000/512 kHz = 93.75 kHz
    #else
        // 
        #define CS_SPEED QUAD_SPEED
        #define MCKL_SCALE 2 // 1: for MCLK = 48 MHz (2: MCLK = 24 MHz (only for FS<100kHz))
        // set MCLK to 48 MHz
        // SCALE 1: 48000/512 kHz = 93.75 kHz
        // 
    #endif

    void adc_init(void)
    {
        uint8_t val[4]={0};

        uint8_t addr = CS536X_I2C_ADDR;
        uint8_t speed = CS_SPEED;

        pinMode(nRST,OUTPUT); digitalWrite(nRST,LOW); 
        delay(1000);
        digitalWrite(nRST,HIGH); 

        i2c_class i2c(&Wire, 100'000, 16, 17);
        if(!i2c.exist(addr)) Serial.println("No I2C address found");

        val[0] = i2c.read(addr, CS536X_REVI);
        i2c.write(addr, CS536X_GCTL, CS536X_GCTL_CP_EN |
                                    CS536X_GCTL_MDIV(CLK_MDIV) | 
                                    CS536X_GCTL_DIF(TDM_MODE) | 
                                    CS536X_GCTL_MODE(speed));
        val[1] = i2c.read(addr, CS536X_GCTL);

        val[2] = i2c.read(addr, CS536X_PDN);
        val[3] = i2c.read(addr, CS536X_nSDOUT);
  
        Serial.printf("CS536x: %x %x %x %x %x\r\n",addr, val[0],val[1],val[2],val[3]);

    //  i2c.write(addr, CS536X_nHPF, 0b01111111);
    //  i2c.write(addr, CS536X_PDN,    0b00001000);
    //  i2c.write(addr, CS536X_nSDOUT, 0b00001000);

    //  i2c.write(addr, CS536X_MUTE, 0b11111110);
    }

    void setAGain(int8_t again) {  }
    void adcStatus(void) {  }

#else
/******************************* TLV320ADC6140 ********************************************/
    #define I2C_ADDRESS1 0x4C // 0-0
    #define I2C_ADDRESS2 0x4D // 0-1
    static const uint8_t i2c_addr[2]= {I2C_ADDRESS1, I2C_ADDRESS2};
    static const uint8_t regs[4]={0x3C, 0x41, 0x46, 0x4B};

    typedef struct
    {
        /* data */
        uint32_t sernum;
        uint8_t chMsk[2];
        uint8_t chMap[2][4];
        uint8_t cfg1[2][4];
        uint8_t cfg2[2][4];
        uint8_t cfg3[2][4];
        uint8_t cfg4[2][4];
    } adc_parameter_t;
    //
    adc_parameter_t adc_parameter[4]=
        {{0x0BA843, {0b1110<<4, 0b1110<<4},
                    {{0,1,2,3},{0,1,2,3}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}}, // CFG1 (line-in 2.5k)
                    {{0,0,0,0},{0,0,0,0}},  // CFG2
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},  // CFG3
                    {{0,0,0,0},{0,0,0,0}}}, // CFG4
         {0x0BAA85, {0b1110<<4, 0b1110<<4},
                    {{0,1,2,3},{0,1,2,3}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},
                    {{0,0,0,0},{0,0,0,0}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},
                    {{0,0,0,0},{0,0,0,0}}},
         {0x0BAAAD, {0b1110<<4, 0b1110<<4},
                    {{0,1,2,3},{0,1,2,3}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},
                    {{0,0,0,0},{0,0,0,0}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},
                    {{0,0,0,0},{0,0,0,0}}},
         {0x0BCA60, {0b1110<<4, 0b1110<<4},
                    {{0,1,2,3},{0,1,2,3}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},
                    {{0,0,0,0},{0,0,0,0}},
                    {{0x80,0x80,0x80,0x80},{0x80,0x80,0x80,0x80}},
                    {{0,0,0,0},{0,0,0,0}}}};

    /*
    # Release SHDNZ to HIGH. 
    # Wait for 1ms. 
    # 
    # Wake-up device by I2C write into P0_R2 using internal AREG 
    w 98 02 81 
    # 
    # Enable Input Ch-1 to Ch-4 by I2C write into P0_R115 
    w 98 73 F0 
    # 
    # Enable ASI Output Ch-1 to Ch-4 slots by I2C write into P0_R116 
    w 98 74 F0 
    # 
    # Power-up ADC, MICBIAS and PLL by I2C write into P0_R117 
    w 98 75 E0 
    # 
    # Apply FSYNC = 44.1 kHz and BCLK = 11.2896 MHz and 
    # Start recording data by host on ASI bus with TDM protocol 32-bits channel wordlengt


    # Wake-up device by I2C write into P0_R2 using internal AREG 
    w 98 02 81 
    # 
    # Configure CH1_INSRC as Digital PDM Input by I2C write into P0_R60 
    w 98 3C 40 
    # 
    # Configure CH2_INSRC as Digital PDM Input by I2C write into P0_R65 
    w 98 41 40 
    # 
    # Configure CH3_INSRC as Digital PDM Input by I2C write into P0_R70 
    w 98 46 40 
    # 
    # Configure CH4_INSRC as Digital PDM Input by I2C write into P0_R75 
    w 98 4B 40 
    # 
    # Configure GPO1 as PDMCLK by I2C write into P0_R34 
    w 98 22 41 
    # 
    # Configure GPO2 as PDMCLK by I2C write into P0_R35 
    w 98 23 41 
    # 
    # Configure GPO3 as PDMCLK by I2C write into P0_R36 
    w 98 24 41 
    # 
    # Configure GPO4 as PDMCLK by I2C write into P0_R37 
    w 98 25 41 
    # 
    # Configure GPI1 and GPI2 as PDMDIN1 and PDMDIN2 by I2C write into P0_R43
    w 98 2B 45 
    # 
    # Configure GPI3 and GPI4 as PDMDIN3 and PDMDIN4 by I2C write into P0_R44 
    w 98 2C 67 
    # 
    # Enable Input Ch-1 to Ch-8 by I2C write into P0_R115 
    w 98 73 FF 
    # 
    # Enable ASI Output Ch-1 to Ch-8 slots by I2C write into P0_R116 
    w 98 74 FF 
    # 
    # Power-up ADC and PLL by I2C write into P0_R117 
    w 98 75 60 
    # 
    # Apply FSYNC = 44.1 kHz and BCLK = 11.2896 MHz and 
    # Start recording data by host on ASI bus with TDM protocol 32-bits channel wordlength

    w 98 0B 00 # Set U1 Ch1 mapped to slot 0 of SDOUT 
    w 98 0C 01 # Set U1 Ch2 mapped to slot 1 of SDOUT 
    w 98 0D 02 # Set U1 Ch3 mapped to slot 2 of SDOUT 
    w 98 0E 03 # Set U1 Ch4 mapped to slot 3 of SDOUT
    */

    void adc_init(void)
    {
        // reset ADC's 
        pinMode(22,OUTPUT);
        digitalWriteFast(22,LOW);  //SHDNZ
        delay(100);
        digitalWriteFast(22,HIGH);  //SHDNZ

        /* ADDRESS L,L: 0x4C ; H,L: 0x4D; L,H: 0x4E; H,H: 0x4F */
        i2c_class i2c(&Wire1,100'000);

        // check existance of device
        for(int ii=0; ii<2; ii++)
        {
            if(i2c.exist(i2c_addr[ii]))
                Serial.printf("found %x\n",i2c_addr[ii]);
            else
                {  Serial.printf("ADC I2C %x not found\n",i2c_addr[ii]); while(1) ;}

            i2c.write(i2c_addr[ii],0x02,0x81); // 1.8V AREG, not sleep

            i2c.write(i2c_addr[ii],0x02,0x81); // 1.8V AREG, not sleep
            i2c.write(i2c_addr[ii],0x07,(3<<4)); // TDM; 32 bit; default clock xmit on rising edge); zero fill
//            i2c.write(i2c_addr[ii],0x07,(3<<4) | (1<<1)); // 32 bit and TX-edge
            i2c.write(i2c_addr[ii],0x08,0x00); // TX_offset 0

////            i2c.write(i2c_addr[ii],0x13,0<<6); // AUTO_CLK_TX
//            i2c.write(i2c_addr[ii],0x16,0x16, (0<<6 | 0<<3));

            for(int jj=0;jj<4;jj++)
            {
                i2c.write(i2c_addr[ii],0x0B+jj,chmap[ii][jj]); 
            }

            i2c.write(i2c_addr[ii],0x73,chanMask[ii]);  
            i2c.write(i2c_addr[ii],0x74,chanMask[ii]);
            i2c.write(i2c_addr[ii],0x75,0x60);

            i2c.write(i2c_addr[ii],0x3B,0x00); // 0: 2.75V; 1: 2.5V; 2: 1.375V

            for(int jj=0; jj<4; jj++)
            {   
                #if ATEST==1
                again=AG[ii][jj];
                #endif

                i2c.write(i2c_addr[ii],regs[jj]+0, 0x88);  // CH1_CFG0 (Line in, 20 kOhm))
                i2c.write(i2c_addr[ii],regs[jj]+1, again); // CH1_CFG1 (0dB gain)
                i2c.write(i2c_addr[ii],regs[jj]+2, 201+dgain);   // CH1_CFG2
                i2c.write(i2c_addr[ii],regs[jj]+3, 0x80);  // CH1_CFG3 (0dB decimal gain correction: +/- 0.8 dB) 
                i2c.write(i2c_addr[ii],regs[jj]+4, 0x00);  // CH1_CFG4 (0bit)
            }
        }
    }

    void setAGain(int8_t again)
    {
        i2c_class i2c(&Wire1,100'000);
        for(int ii=0; ii<2; ii++)
            for(int jj=0; jj<4; jj++)
            {
                i2c.write(i2c_addr[ii],regs[jj]+1, again); // CH1_CFG1 (0dB gain)
            }
    }
    void adcStatus(void)
    {
        i2c_class i2c(&Wire1,100'000);
        for(int ii=0; ii<2; ii++)
        { Serial.print("\n ADC 0x15: "); Serial.print(i2c.read(i2c_addr[ii],0x15),HEX);
        }
        Serial.println();
    }
#endif
