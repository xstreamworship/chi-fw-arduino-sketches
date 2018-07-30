/////////////////////////////////////////////////////////////////////
// Analogue I/O driver for AD7997 8 channel ADC.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __AD7997_H
#define __AD7997_H

#include "Arduino.h"

class CAd7997
{
  public:
    enum properties
    {
      E_NUM_PORTS = 8,
    };
    
  private:
    enum cmd_reg
    {
      ECREG_ADDR = 2,
      ECREG_MSB = 0x0f,
      ECREG_LSB = 0xf8,
      E_READ_CMD = 0x80,
    };
    const uint8_t m_i2cAddr;
    uint16_t m_inValue[E_NUM_PORTS];
    unsigned m_errCnt;

  public:
    CAd7997(const uint8_t i2cAddr);
    ~CAd7997(void);
    void begin(void);
    void sync(void);
    void sync(uint8_t index);
    uint16_t read(uint8_t index) { return m_inValue[index]; }

  public:
    static uint8_t lsb_of(uint16_t val) { return val & 0xff; }
    static uint8_t msb_of(uint16_t val) { return (val >> 8) & 0xff; }
    static void set_lsb(uint16_t &targ, uint8_t val) { targ = (targ & 0xff00) | (val & 0x00ff); }
    static void set_msb(uint16_t &targ, uint8_t val) { targ = (targ & 0x00ff) | (val << 8) & 0xff00; }
};

#endif

