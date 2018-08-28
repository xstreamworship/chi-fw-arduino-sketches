/////////////////////////////////////////////////////////////////////
// Digital I/O driver for CAT9555 16 line port.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __CAT9555_H
#define __CAT9555_H

#include "Arduino.h"
#include "twi_if.h"

struct __attribute__ ((packed)) SCat9555Buf
{
  uint8_t cmd;
  uint8_t port;
};

class CCat9555
{
  private:
    enum cmd_reg
    {
      ECREG_INPUT = 0,
      ECREG_OUTPUT = 2,
      ECREG_IN_INV = 4,
      ECREG_PORT_CONFIG = 6,
    };
    enum properties
    {
      E_NUM_PORTS = 2,
    };
    uint32_t m_readAt;
    SCat9555Buf m_in;
    SCat9555Buf m_out;
    const uint8_t m_portConfig;
    const uint8_t m_i2cAddr;
    const uint8_t m_portNum;

  public:
    CCat9555(const uint8_t i2cAddr, const uint8_t portNum, const uint8_t portConfig);
    ~CCat9555(void);
    void begin(uint8_t outValue = 0xff);
    void startOut(void);
    void startIn(void);
    void syncOut(void);
    void syncIn(void);
    bool isBusy(void) { return twi_busy(); }
    inline uint32_t syncdInAt(void) { return m_readAt; }
    inline void write(uint8_t outValue) { m_out.port = outValue; }
    inline uint16_t read(void) { return (m_in.port & m_portConfig) | (m_out.port & ~ m_portConfig); }
};

#endif

