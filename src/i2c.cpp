#include "i2c.h"


/*
uint8_t i2c_exist(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return (Wire.endTransmission()==0);
}

uint8_t i2c_read(uint8_t addr, uint8_t reg)
{
    unsigned int val;
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0;
    if (Wire.requestFrom((int)addr, 1) < 1) return 0;
    val = Wire.read();
    return val;
}

bool i2c_write(uint8_t addr, uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

uint8_t i2c1_exist(uint8_t addr)
{
    Wire1.beginTransmission(addr);
    return (Wire1.endTransmission()==0);
}

uint8_t i2c1_read(uint8_t addr, uint8_t reg)
{
    unsigned int val;
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return 0;
    if (Wire1.requestFrom((int)addr, 1) < 1) return 0;
    val = Wire1.read();
    return val;
}

bool i2c1_write(uint8_t addr, uint8_t reg, uint8_t val)
{
    Wire1.beginTransmission(addr);
    Wire1.write(reg);
    Wire1.write(val);
    return (Wire1.endTransmission() == 0);
}
*/

#if 0
void test_wire0(void)
{
        Wire.begin();
        delay(100);
//        Wire.setSDA(18);
//        Wire.setSCL(19);
        Wire.setClock(100'000);

    for(int ii=0;ii<127;ii++)
    {
        Wire.beginTransmission(ii);
        uint8_t error = Wire.endTransmission();
        if(error) 
            Serial.printf("I2C not found %x\n",ii);  
        else 
            Serial.printf("I2C found %x\n",ii);      
    }
    while(1);  
}

void test_wire1(void)
{
        Wire1.begin();
        delay(100);
        Wire1.setSDA(17);
        Wire1.setSCL(16);
        Wire1.setClock(100'000);

    for(int ii=0;ii<127;ii++)
    {
        Wire1.beginTransmission(ii);
        uint8_t error = Wire1.endTransmission();
        if(error) 
            Serial.printf("I2C not found %x\n",ii);  
        else 
            Serial.printf("I2C found %x\n",ii);      
    }
    while(1);  
}
#endif