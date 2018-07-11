/////////////////////////////////////////////////////////////////////
// Translates keyboard switch inputs into note on/off like events.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __MIDIKEYSWITCH_H
#define __MIDIKEYSWITCH_H

#include "Arduino.h"

class CMidiKeySwitchS
{
  public:
    CMidiKeySwitchS(void);
    ~CMidiKeySwitchS(void);
    enum state
    {
      E_NOTE_OFF,
      E_NC_OPENED,
      E_NOTE_ON,
      E_NO_OPENED,
    };
    enum state m_state;
    unsigned long m_timeStart;
    unsigned long m_timeTransition;
    uint8_t m_velocity;
    //unsigned m_errCount;
};

class CMidiKeySwitch
{
  private:
    const PROGMEM uint8_t m_note;
    const PROGMEM uint8_t m_noteOctave;
    const PROGMEM char *m_noteName;
    enum properties
    {
      E_DEFAULT_VELOCITY = 64
    };
    CMidiKeySwitchS * const PROGMEM m_S;

  public:
    CMidiKeySwitch(const PROGMEM uint8_t note, const PROGMEM char *noteName, const PROGMEM uint8_t noteOctave);
    CMidiKeySwitch(const PROGMEM uint8_t note, const PROGMEM char *noteName, const PROGMEM uint8_t noteOctave, const PROGMEM CMidiKeySwitchS * css);
    ~CMidiKeySwitch(void);
    void scan(bool nc, bool no);
    void noteOn(void);
    void noteOff(void);
};

#endif
