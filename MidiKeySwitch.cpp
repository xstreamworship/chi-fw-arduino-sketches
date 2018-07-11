/////////////////////////////////////////////////////////////////////
// Translates keyboard switch inputs into note on/off like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#include "Arduino.h"
#include "MidiKeySwitch.h"

CMidiKeySwitchS::CMidiKeySwitchS(void) :
  m_state(E_NOTE_OFF),
  m_timeStart(0),
  m_timeTransition(0),
  //m_errCount(0),
  m_velocity(0)
{
}

CMidiKeySwitchS::~CMidiKeySwitchS(void)
{
}

CMidiKeySwitch::CMidiKeySwitch(const PROGMEM uint8_t note, const PROGMEM char *noteName, const PROGMEM uint8_t noteOctave) :
  m_note(note),
  m_noteName(noteName),
  m_noteOctave(noteOctave),
  m_S(new CMidiKeySwitchS())
{
}

CMidiKeySwitch::CMidiKeySwitch(const PROGMEM uint8_t note, const PROGMEM char *noteName, const PROGMEM uint8_t noteOctave, const PROGMEM CMidiKeySwitchS * css) :
  m_note(note),
  m_noteName(noteName),
  m_noteOctave(noteOctave),
  m_S(css)
{
}

CMidiKeySwitch::~CMidiKeySwitch(void)
{
}

void CMidiKeySwitch::scan(bool nc, bool no)
{
  if (nc)
  {
    // When NC active, note is (or just went) off
    if (no)
    {
      // Illegal, switch cannot be both NC and NO.
      //m_S->m_errCount++;
      return;
    }
    switch (m_S->m_state)
    {
      case CMidiKeySwitchS::E_NOTE_OFF:
        // Do nothing.
        break;
      case CMidiKeySwitchS::E_NC_OPENED:
        // False note-on - cancel it.
        m_S->m_state = CMidiKeySwitchS::E_NOTE_OFF;
        break;
      case CMidiKeySwitchS::E_NOTE_ON:
      default:
        // Rapid transition from note on to note off.
        m_S->m_timeStart = micros();
        // fall through
      case CMidiKeySwitchS::E_NO_OPENED:
        // Complete the note-off
        m_S->m_timeTransition = micros() - m_S->m_timeStart;
        noteOff();
        m_S->m_state = CMidiKeySwitchS::E_NOTE_OFF;
        break;
    }
  }
  else if (no)
  {
    switch (m_S->m_state)
    {
      case CMidiKeySwitchS::E_NOTE_OFF:
      default:
        // Rapid transition from note off to note on.
        m_S->m_timeStart = micros();
        // fall through
      case CMidiKeySwitchS::E_NC_OPENED:
        // Complete the note-on.
        m_S->m_timeTransition = micros() - m_S->m_timeStart;
        noteOn();
        m_S->m_state = CMidiKeySwitchS::E_NOTE_ON;
        break;
      case CMidiKeySwitchS::E_NOTE_ON:
        // Do nothing.
        break;
      case CMidiKeySwitchS::E_NO_OPENED:
        // False note-off - cancel it.
        m_S->m_state = CMidiKeySwitchS::E_NOTE_ON;
        break;
    }
  }
  else
  {
    switch (m_S->m_state)
    {
      case CMidiKeySwitchS::E_NOTE_OFF:
        // Initiate a note-on.
        m_S->m_timeStart = micros();
        m_S->m_state = CMidiKeySwitchS::E_NC_OPENED;
        break;
      case CMidiKeySwitchS::E_NC_OPENED:
      case CMidiKeySwitchS::E_NO_OPENED:
      default:
        // Do nothing.
        break;
      case CMidiKeySwitchS::E_NOTE_ON:
        // Initiate a note-off.
        m_S->m_timeStart = micros();
        m_S->m_state = CMidiKeySwitchS::E_NO_OPENED;
        break;
        break;
    }
  }
}

void CMidiKeySwitch::noteOn(void)
{
  char mbuffer[10];
  Serial.print(F("Note: "));
  strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.print(m_noteOctave);
  Serial.print(F(" on: "));
  Serial.println(m_S->m_timeTransition);
}

void CMidiKeySwitch::noteOff(void)
{
  char mbuffer[10];
  Serial.print(F("Note: "));
  strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.print(m_noteOctave);
  Serial.print(F(" off: "));
  Serial.println(m_S->m_timeTransition);
}
