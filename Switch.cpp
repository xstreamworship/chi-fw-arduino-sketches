/////////////////////////////////////////////////////////////////////
// Translate switch input samples into CC like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"
#include "Switch.h"

CSwitchS::CSwitchS(void) :
  m_state(E_SW_OFF),
  m_timeStart(0),
  m_ledState(false)
{
}

CSwitchS::~CSwitchS(void)
{
}

CSwitch::CSwitch(const PROGMEM char *switchName, const PROGMEM unsigned long timeDebounce) :
  m_switchName(switchName),
  m_timeDebounce(timeDebounce),
  m_S(new CSwitchS())
{
}

CSwitch::CSwitch(const PROGMEM char *switchName, const PROGMEM unsigned long timeDebounce, const PROGMEM CSwitchS * css) :
  m_switchName(switchName),
  m_timeDebounce(timeDebounce),
  m_S(css)
{
}

CSwitch::~CSwitch(void)
{
}

void CSwitch::scan(bool state)
{
  if (state)
  {
    // On or just turned on.
    switch (m_S->m_state)
    {
      case CSwitchS::E_SW_ON:
        // Do nothing.
        break;
      case CSwitchS::E_SW_VALIDATING_OFF:
        // False off - cancel it.
        m_S->m_state = CSwitchS::E_SW_ON;
        break;
      case CSwitchS::E_SW_OFF:
      default:
        // Transition from off to on.
        m_S->m_timeStart = micros();
        m_S->m_state = CSwitchS::E_SW_VALIDATING_ON;
        // fall through
      case CSwitchS::E_SW_VALIDATING_ON:
        // Complete the off
        if ((micros() - m_S->m_timeStart) >= m_timeDebounce) {
          switchOn();
          m_S->m_state = CSwitchS::E_SW_ON;
        }
        break;
    }
  }
  else
  {
    // Off or just turned off.
    switch (m_S->m_state)
    {
      case CSwitchS::E_SW_OFF:
        // Do nothing.
        break;
      case CSwitchS::E_SW_VALIDATING_ON:
        // False on - cancel it.
        m_S->m_state = CSwitchS::E_SW_OFF;
        break;
      case CSwitchS::E_SW_ON:
      default:
        // Transition from on to off.
        m_S->m_timeStart = micros();
        m_S->m_state = CSwitchS::E_SW_VALIDATING_OFF;
        // fall through
      case CSwitchS::E_SW_VALIDATING_OFF:
        // Complete the off
        if ((micros() - m_S->m_timeStart) >= m_timeDebounce) {
          switchOff();
          m_S->m_state = CSwitchS::E_SW_OFF;
        }
        break;
    }
  }
}

void CSwitch::switchOn(void)
{
  char mbuffer[12];
  Serial.print(F("Switch: "));
  strncpy_P(mbuffer, m_switchName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.println(F(" on"));
}

void CSwitch::switchOff(void)
{
  char mbuffer[12];
  Serial.print(F("Switch: "));
  strncpy_P(mbuffer, m_switchName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.println(F(" off"));
}

