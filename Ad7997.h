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
#include "twi_if.h"

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
      E_READ_ALL_CMD = 0x70,
    };
    const uint8_t m_i2cAddr;
    uint8_t m_inBytes[E_NUM_PORTS * 2];
    unsigned m_errCnt;

  public:
    CAd7997(const uint8_t i2cAddr);
    ~CAd7997(void);
    void begin(void);
    void start(void);
    void start(uint8_t index);
    void sync(void);
    void sync(uint8_t index);
    bool isBusy(void) { return twi_busy(); }
    uint16_t read(uint8_t index) {
      if (((m_inBytes[2 * index] >> 4) & 0x07) != index) {
        // Error - TODO - FIXME - have error handling.
        m_errCnt++;
        return 0;
      }
      // Note: Big endian from the AD7997.
      return ((m_inBytes[2 * index] & 0x0f) << 8) + m_inBytes[(2 * index) + 1];
    }
};

#endif

