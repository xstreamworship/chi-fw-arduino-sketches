/////////////////////////////////////////////////////////////////////
// Translate switch input samples into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __SWITCH_H
#define __SWITCH_H

#include "Arduino.h"

class CSwitch
{
  private:
    enum state
    {
      E_SW_OFF,
      E_SW_VALIDATING_ON,
      E_SW_ON,
      E_SW_VALIDATING_OFF,
    };
    unsigned long m_timeStart;
    enum state m_state;
    uint8_t m_ledState;
    char * const PROGMEM m_switchName;
    enum
    {
      E_TIMEDEBOUNCE = 30000,
    };
    void switchOn(uint8_t ccNum, uint8_t uCase);
    void switchOff(uint8_t ccNum, uint8_t uCase);

  public:
    CSwitch(const PROGMEM char *switchName);
    ~CSwitch(void);
    void scan(bool state, uint8_t ccNum = 255, uint8_t uCase = 0);
    inline bool ledState(uint8_t ovLay=0) { return (m_ledState & (1 << (ovLay & 0x07))) != 0; }
    inline void setLedState(bool state, uint8_t ovLay=0) { if (state) ledOn(ovLay); else ledOff(ovLay); }
    inline void ledOn(uint8_t ovLay=0) { m_ledState |= (1 << (ovLay & 0x07)); }
    inline void ledOff(uint8_t ovLay=0) { m_ledState &= ~(1 << (ovLay & 0x07)); }
};

#endif

