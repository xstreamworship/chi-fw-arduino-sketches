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

class CSwitchS
{
  public:
    CSwitchS(void);
    ~CSwitchS(void);
    enum state
    {
      E_SW_OFF,
      E_SW_VALIDATING_ON,
      E_SW_ON,
      E_SW_VALIDATING_OFF,
    };
    enum state m_state;
    unsigned long m_timeStart;
    bool m_ledState;
};

class CSwitch
{
  private:
    const PROGMEM char *m_switchName;
    const PROGMEM unsigned long m_timeDebounce;
    CSwitchS * const PROGMEM m_S;

  public:
    CSwitch(const PROGMEM char *switchName, const PROGMEM unsigned long timeDebounce);
    CSwitch(const PROGMEM char *switchName, const PROGMEM unsigned long timeDebounce, const PROGMEM CSwitchS * css);
    ~CSwitch(void);
    void scan(bool state);
    void switchOn(void);
    void switchOff(void);
    bool ledState(void) { return m_S->m_ledState; }
    void setLedState(bool state) { m_S->m_ledState = state; }
    void ledOn(void) { setLedState(true); }
    void ledOff(void) { setLedState(false); }
};

#endif
