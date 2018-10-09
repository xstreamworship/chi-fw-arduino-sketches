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

extern bool debug_mode;
void noteOn(uint8_t note, uint8_t velocity, uint8_t channel);
void noteOff(uint8_t note, uint8_t velocity, uint8_t channel);

unsigned long CMidiKeySwitch::s_glitchCount = 0;
uint8_t CMidiKeySwitch::s_midi_ch = 0;

CMidiKeySwitch::CMidiKeySwitch(const char * const PROGMEM noteName) :
  m_noteName(noteName),
  m_state(E_NOTE_OFF),
  m_timeStart(0)
{
}

CMidiKeySwitch::~CMidiKeySwitch(void)
{
}

void CMidiKeySwitch::scan(uint32_t sTime, uint8_t key, bool nc, bool no)
{
  uint8_t note = key + E_NOTE_A1_OFFSET;
  if (nc)
  {
    // When NC active, note is (or just went) off
    if (no)
    {
      // Illegal, switch cannot be both NC and NO.
      s_glitchCount++;
      stateError(note, sTime, key, nc, no);
      return;
    }
    switch (m_state)
    {
      case E_NOTE_OFF:
        // Do nothing but refresh start time.
        m_timeStart = sTime;
        break;
      case E_NC_OPENED:
        // False note-on - cancel it.
        m_state = E_NOTE_OFF;
        // Refresh start time.
        m_timeStart = sTime;
        break;
      default:
        stateError(note, sTime, key, nc, no);
        // fall through
      case E_NOTE_ON:
        // Rapid transition from note on to note off.
        // Note: Relying on the last refreshed start time.
        s_glitchCount++;
        // fall through
      case E_NO_OPENED:
        // Complete the note-off
        noteOff(note, sTime - m_timeStart);
        m_state = E_NOTE_OFF;
        // Refresh start time.
        m_timeStart = sTime;
        break;
    }
  }
  else if (no)
  {
    switch (m_state)
    {
      default:
        stateError(note, sTime, key, nc, no);
        // fall through
      case E_NOTE_OFF:
        // Rapid transition from note off to note on.
        // Note: Relying on the last refreshed start time.
        s_glitchCount++;
        // fall through
      case E_NC_OPENED:
        // Complete the note-on.
        noteOn(note, sTime - m_timeStart);
        m_state = E_NOTE_ON;
        // Refresh start time.
        m_timeStart = sTime;
        break;
      case E_NOTE_ON:
        // Do nothing but refresh start time.
        m_timeStart = sTime;
        break;
      case E_NO_OPENED:
        // False note-off - cancel it.
        m_state = E_NOTE_ON;
        // Refresh start time.
        m_timeStart = sTime;
        break;
    }
  }
  else
  {
    switch (m_state)
    {
      case E_NOTE_OFF:
        // Initiate a note-on with properly scanned start time.
        m_timeStart = sTime;
        m_state = E_NC_OPENED;
        break;
      default:
        stateError(note, sTime, key, nc, no);
        // fall through
      case E_NC_OPENED:
      case E_NO_OPENED:
        // Do nothing because this is normal transitional swing.
        break;
      case E_NOTE_ON:
        // Initiate a note-off with properly scanned start time.
        m_timeStart = sTime;
        m_state = E_NO_OPENED;
        break;
        break;
    }
  }
}

extern void pulseLed(void);

#define TTIMEMIN 1000
#define MNDENOM1 1024
#define MNDENOM2 256
void CMidiKeySwitch::noteOn(uint8_t note, unsigned long ttime)
{
  uint8_t vel;
  if (ttime <= TTIMEMIN)
    vel = 127;
  else {
    unsigned long ttime_scaled =
        ((ttime - TTIMEMIN) / MNDENOM1) +
        ((ttime - TTIMEMIN) / MNDENOM2);
    if (ttime_scaled >= 126)
      vel = 1;
    else
      vel = 127 - ttime_scaled;
  }

  ::noteOn(note, vel, s_midi_ch);
  if (debug_mode) {
    char mbuffer[10];
    strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    Serial.print(F(" "));
    Serial.print(ttime);
    Serial.print(F(" : "));
    Serial.println(vel);
  }
}

extern unsigned long cycleTime;

void CMidiKeySwitch::noteOff(uint8_t note, unsigned long ttime)
{
  uint8_t vel = 0;
  ::noteOff(note, vel, s_midi_ch);
  if (debug_mode) {
    char mbuffer[10];
    strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    Serial.print(F(" off: "));
    Serial.print(ttime);
    Serial.print(F("[ "));
    Serial.print(s_glitchCount);
    Serial.print(F(", "));
    Serial.print(cycleTime / 8);
    Serial.println(F(" ]"));
  }
}

void CMidiKeySwitch::stateError(uint8_t note, unsigned long ttime, uint8_t key, bool nc, bool no)
{
  if (debug_mode) {
    char mbuffer[10];
    Serial.print(F("Note: "));
    Serial.print(note);
    Serial.print(F(": "));
    strncpy_P(mbuffer, m_noteName, sizeof(mbuffer) - 1);
    Serial.print(mbuffer);
    Serial.print(F(" error: "));
    Serial.print(ttime);
    Serial.print(F("[ "));
    Serial.print(key);
    Serial.print(F(", "));
    Serial.print(nc);
    Serial.print(F(", "));
    Serial.print(no);
    Serial.print(F(", "));
    Serial.print(s_glitchCount);
    Serial.println(F(" ]"));
  }
}

