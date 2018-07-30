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
    static uint8_t s_midi_ch;
    static unsigned long s_glitchCount;
    void noteOn(uint8_t note, unsigned long ttime);
    void noteOff(uint8_t note, unsigned long ttime);
    uint8_t velInterp(unsigned long ttime, unsigned long toptime, unsigned long bottime, uint8_t topvel, uint8_t botvel)
    {
      return ((ttime - bottime) * (topvel - botvel) / (toptime - bottime)) + botvel;
    }
    void stateError(uint8_t note, unsigned long ttime, uint8_t key, bool nc, bool no);

  public:
    CMidiKeySwitch(char * const PROGMEM noteName);
    ~CMidiKeySwitch(void);
    static inline uint8_t getMidiCh(void) { return s_midi_ch; }
    void scan(uint8_t key, bool nc, bool no);
};

#endif

