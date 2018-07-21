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

CMidiKeySwitch::CMidiKeySwitch(char * const PROGMEM noteName) :
  m_noteName(noteName),
  m_state(E_NOTE_OFF),
  m_timeStart(0)
{
}

CMidiKeySwitch::~CMidiKeySwitch(void)
{
}

void CMidiKeySwitch::scan(uint8_t key, bool nc, bool no)
{
  uint8_t note = key + E_NOTE_A1_OFFSET;
  if (nc)
  {
    // When NC active, note is (or just went) off
    if (no)
    {
      // Illegal, switch cannot be both NC and NO.
      return;
    }
    switch (m_state)
    {
      case E_NOTE_OFF:
        // Do nothing.
        break;
      case E_NC_OPENED:
        // False note-on - cancel it.
        m_state = E_NOTE_OFF;
        break;
      case E_NOTE_ON:
      default:
        // Rapid transition from note on to note off.
        m_timeStart = micros();
        // fall through
      case E_NO_OPENED:
        // Complete the note-off
        noteOff(note, micros() - m_timeStart);
        m_state = E_NOTE_OFF;
        break;
    }
  }
  else if (no)
  {
    switch (m_state)
    {
      case E_NOTE_OFF:
      default:
        // Rapid transition from note off to note on.
        m_timeStart = micros();
        // fall through
      case E_NC_OPENED:
        // Complete the note-on.
        noteOn(note, micros() - m_timeStart);
        m_state = E_NOTE_ON;
        break;
      case E_NOTE_ON:
        // Do nothing.
        break;
      case E_NO_OPENED:
        // False note-off - cancel it.
        m_state = E_NOTE_ON;
        break;
    }
  }
  else
  {
    switch (m_state)
    {
      case E_NOTE_OFF:
        // Initiate a note-on.
        m_timeStart = micros();
        m_state = E_NC_OPENED;
        break;
      case E_NC_OPENED:
      case E_NO_OPENED:
      default:
        // Do nothing.
        break;
      case E_NOTE_ON:
        // Initiate a note-off.
        m_timeStart = micros();
        m_state = E_NO_OPENED;
        break;
        break;
    }
  }
}

void CMidiKeySwitch::noteOn(uint8_t note, unsigned long ttime)
{
  char mbuffer[10];
  Serial.print(F("Note: "));
  Serial.print(note);
  Serial.print(F(": "));
  strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.print(F(" on: "));
  Serial.println(ttime);
}

void CMidiKeySwitch::noteOff(uint8_t note, unsigned long ttime)
{
  char mbuffer[10];
  Serial.print(F("Note: "));
  Serial.print(note);
  Serial.print(F(": "));
  strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
  Serial.print(mbuffer);
  Serial.print(F(" off: "));
  Serial.println(ttime);
}

