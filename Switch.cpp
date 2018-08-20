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

extern bool debug_mode;
void switchChanged(bool state, uint8_t ccNum, uint8_t uCase);

CSwitch::CSwitch(const PROGMEM char *switchName) :
  m_switchName(switchName),
  m_state(E_SW_OFF),
  m_stateStatus(false),
  m_timeStart(0),
  m_ledState(false)
{
}

CSwitch::~CSwitch(void)
{
}

void CSwitch::scan(uint32_t sTime, bool state, uint8_t ccNum, uint8_t uCase)
{
  if (state)
  {
    // On or just turned on.
    switch (m_state)
    {
      case E_SW_ON:
        // Do nothing.
        break;
      case E_SW_VALIDATING_OFF:
        // False off - cancel it.
        m_state = E_SW_ON;
        break;
      case E_SW_OFF:
      default:
        // Transition from off to on.
        m_timeStart = sTime;
        m_state = E_SW_VALIDATING_ON;
        // fall through
      case E_SW_VALIDATING_ON:
        // Complete the off
        if ((sTime - m_timeStart) >= E_TIMEDEBOUNCE) {
          switchOn(ccNum, uCase);
          m_state = E_SW_ON;
          m_stateStatus = true;
        }
        break;
    }
  }
  else
  {
    // Off or just turned off.
    switch (m_state)
    {
      case E_SW_OFF:
        // Do nothing.
        break;
      case E_SW_VALIDATING_ON:
        // False on - cancel it.
        m_state = E_SW_OFF;
        break;
      case E_SW_ON:
      default:
        // Transition from on to off.
        m_timeStart = sTime;
        m_state = E_SW_VALIDATING_OFF;
        // fall through
      case E_SW_VALIDATING_OFF:
        // Complete the off
        if ((sTime - m_timeStart) >= E_TIMEDEBOUNCE) {
          switchOff(ccNum, uCase);
          m_state = E_SW_OFF;
          m_stateStatus = false;
        }
        break;
    }
  }
}

void CSwitch::switchOn(uint8_t ccNum, uint8_t uCase)
{
  if (!debug_mode) {
    // Using serial for MIDI.
    switchChanged(true, ccNum, uCase);
  } else {
    char mbuffer[12];
    Serial.print(F("Switch: "));
    strncpy_P(mbuffer, m_switchName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    Serial.println(F(" on"));
  }
}

void CSwitch::switchOff(uint8_t ccNum, uint8_t uCase)
{
  if (!debug_mode) {
    // Using serial for MIDI.
    switchChanged(false, ccNum, uCase);
  } else {
    char mbuffer[12];
    Serial.print(F("Switch: "));
    strncpy_P(mbuffer, m_switchName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    Serial.println(F(" off"));
  }
}

