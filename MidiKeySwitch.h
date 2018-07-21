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

class CMidiKeySwitch
{
  private:
    unsigned long m_timeStart;
    char * const PROGMEM m_noteName;
    enum properties
    {
      E_DEFAULT_VELOCITY = 64,
      E_NOTE_A1_OFFSET = 45,
    };
    enum state
    {
      E_NOTE_OFF,
      E_NC_OPENED,
      E_NOTE_ON,
      E_NO_OPENED,
    };
    enum state m_state;

  public:
    CMidiKeySwitch(char * const PROGMEM noteName);
    ~CMidiKeySwitch(void);
    void scan(uint8_t key, bool nc, bool no);
    void noteOn(uint8_t note, unsigned long ttime);
    void noteOff(uint8_t note, unsigned long ttime);
};

#endif
