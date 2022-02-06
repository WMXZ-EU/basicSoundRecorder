#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <Wire.h>

class i2c_class
{ TwoWire *wire;
    public:

    i2c_class(TwoWire *wire) 
    {   this->wire = wire;
        wire->begin();
    }

    i2c_class(TwoWire *wire, uint32_t speed) 
    {   this->wire = wire;
        wire->begin();
        delay(100);
        wire->setClock(speed);
    }

    i2c_class(TwoWire *wire, uint32_t speed, uint8_t scl, uint8_t sda) 
    {   this->wire = wire;
        wire->begin();
        delay(100);
        wire->setClock(speed);
        wire->setSCL(scl);
        wire->setSDA(sda);
    }

    uint8_t exist(uint8_t addr)
    {
        wire->beginTransmission(addr);
        return (wire->endTransmission()==0);
    }

    uint8_t read(uint8_t addr, uint8_t reg) 
    { 
        unsigned int val;
        wire->beginTransmission(addr);
        wire->write(reg);
        if (wire->endTransmission(false) != 0) return 0;
        if (wire->requestFrom((int)addr, 1) < 1) return 0;
        val = wire->read();
        return val;
    }

    uint8_t write(uint8_t addr, uint8_t reg) 
    { 
        wire->beginTransmission(addr);
        wire->write(reg);
        return (wire->endTransmission() == 0) ;
    }

    uint8_t write(uint8_t addr, uint8_t reg, uint8_t val) 
    { 
        wire->beginTransmission(addr);
        wire->write(reg);
        wire->write(val);
        return (wire->endTransmission() == 0) ;
    }

    uint16_t read16(uint8_t addr, uint16_t reg) 
    { 
        unsigned int val;
        wire->beginTransmission(addr);
        wire->write(reg >> 8);
        wire->write(reg);
        if (wire->endTransmission(false) != 0) return 0;
        if (wire->requestFrom((int)addr, 1) < 1) return 0;
        val = wire->read()<<8;
        val |= wire->read();
        return val;
    }
    
    uint8_t write16(uint8_t addr, uint16_t reg, uint16_t val) 
    { 
        wire->beginTransmission(addr);
        wire->write(reg >> 8);
        wire->write(reg);
        wire->write(val >> 8);
        wire->write(val);
        return (wire->endTransmission() == 0) ;
    }

    uint8_t *readData( uint8_t addr, uint8_t reg, uint8_t *data, uint8_t ndat)
    {
            wire->beginTransmission(addr);
            wire->write(reg);
            if (wire->endTransmission(false) != 0) return 0;
            if (wire->requestFrom((int)addr, (int)ndat) < 1) return 0;
            for(int ii=0;ii<ndat;ii++) data[ii] = wire->read();
            return data;
    }    

    uint8_t *readData( uint8_t addr, uint8_t reg, uint8_t *data, uint8_t ndat, uint16_t dt)
    {
            wire->beginTransmission(addr);
            wire->write(reg);
            if (wire->endTransmission(false) != 0) return 0;
            delay(dt);
            if (wire->requestFrom((int)addr, (int)ndat) < 1) return 0;
            for(int ii=0;ii<ndat;ii++) data[ii] = wire->read();
            return data;
    }    
};
#endif