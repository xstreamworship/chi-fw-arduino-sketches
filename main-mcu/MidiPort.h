/////////////////////////////////////////////////////////////////////
// Simple MIDI port implementation using Serial.
//
// Copyright 2018, Darcy Watkins
// All Rights Reserved.
//
// Available under Mozilla Public License Version 2.0
// See the LICENSE file for license terms.
/////////////////////////////////////////////////////////////////////
#ifndef __MIDIPORT_H
#define __MIDIPORT_H

#include "Arduino.h"

template<class SerialPort>
class CMidiPort
{
  private:
    SerialPort& m_serial;
    bool m_running;
    bool m_runningStatus;
    void (* m_cbSetLed) (bool);
    void (* m_handleRxMidi) (uint8_t, uint8_t, uint8_t, uint8_t);
    uint8_t m_rxRunningStatus;
    uint8_t m_txRunningStatus;
    uint8_t m_rxData1;
    uint8_t m_txCable;
    uint8_t m_rxCable;
    bool m_rxEscape;
    enum state
    {
      E_MS_
    };
    bool parseMidi(uint8_t rxByte, uint8_t channel)
    {
      bool rc = false;
      uint8_t rxStatus = rxByte & 0xf0;
      uint8_t rxChannel = rxByte & 0x0f;
      if (rxByte < 0x80) {
        // A data byte
        if (m_rxRunningStatus == 0xb0) {
          if (m_rxData1 != 255) {
            // Invoke the callback.
            if (m_handleRxMidi)
              m_handleRxMidi(0xb0, m_rxData1, rxByte, channel);
            m_rxData1 = 255;
            rc = true;
          } else {
            // Store the received CC number
            m_rxData1 = rxByte;
          }
        } else if (m_rxRunningStatus == 0xc0) {
          // Invoke the callback.
          if (m_handleRxMidi)
            m_handleRxMidi(0xc0, rxByte, 0, channel);
          m_rxRunningStatus = 255;
          rc = true;
        } else if (m_rxRunningStatus == 0xe0) {
          if (m_rxData1 != 255) {
            // Invoke the callback.
            if (m_handleRxMidi)
              m_handleRxMidi(0xe0, m_rxData1, rxByte, channel);
            m_rxData1 = 255;
            rc = true;
          } else {
            // Store the received byte.
            m_rxData1 = rxByte;
          }
        } else {
          // Ignore the data byte.
        }
      } else if (rxByte >= 0xf8) {
        // A real time message - ignore it.
      } else if ((rxStatus == 0xb0) && (rxChannel == channel)) {
        // A CC status byte
        m_rxRunningStatus = 0xb0;
        m_rxData1 = 255;
      } else if ((rxStatus == 0xc0) && (rxChannel == channel)) {
        // A PC status byte
        m_rxRunningStatus = 0xc0;
      } else if ((rxStatus == 0xe0) && (rxChannel == channel)) {
        // A PB status byte
        m_rxRunningStatus = 0xe0;
        m_rxData1 = 255;
      } else {
        // Unrecognized / unsupported message or on a different channel.
        m_rxRunningStatus = 255;
        m_rxData1 = 255;
      }
      return rc;
    }
  public:
    inline CMidiPort(SerialPort& serialPort) :
      m_serial(serialPort),
      m_cbSetLed(0),
      m_handleRxMidi(0),
      m_txRunningStatus(0),
      m_rxRunningStatus(255),
      m_rxData1(255),
      m_txCable(0),
      m_rxCable(0),
      m_runningStatus(false),
      m_rxEscape(false),
      m_running(false) { }
    inline ~CMidiPort(void) { }
    inline void begin(void (* cbSetLed) (bool),
      void (* cbRxMidi) (uint8_t, uint8_t, uint8_t, uint8_t) = 0,
      uint32_t rate = 31250)
    {
      m_serial.begin(rate);
      m_running = true;
      m_cbSetLed = cbSetLed;
      m_handleRxMidi = cbRxMidi;
    }
    inline void write(uint8_t data, uint8_t cable = 0)
    {
      if (cable != m_txCable) {
        // Not currently selected cable, so insert the escape sequence.
        m_txCable = (cable & 0x0f);
        m_serial.write(0xfd);
        m_serial.write(0x00 | m_txCable);
      }
      m_serial.write(data);
    }
    inline void write(uint8_t *buf, size_t len, uint8_t cable = 0)
    {
      if (len--) {
        write(*(buf++), cable);
        m_serial.write(buf, len);
      }
    }
    inline bool available(uint16_t cableMask = 0xffff)
    {
      if ((cableMask & (1 << m_rxCable)) == 0)
        return;
      return (m_serial.available() > 0);
    }
    inline size_t read(uint8_t *buf, size_t blen, uint16_t cableMask = 0xffff)
    {
      size_t count = 0;
      if ((cableMask & (1 << m_rxCable)) == 0)
        return 0;
      while (m_serial.available() && blen) {
        uint8_t data = m_serial.read();
        if (m_rxEscape) {
          // In escape sequence.
          m_rxEscape = false;
          if (data == 0xfd) {
            // Escaped the 0xfd value itself.
            *(buf++) = data;
            blen--;
            count++;
          } else if ((data & 0xf0) == 0) {
            // Cable change escape sequence.
            m_rxCable = (data & 0x0f);
            return count;
          }
          // Otherwise ignore the unrecognized escape sequence.
        } else if (data == 0xfd) {
          // start of escape sequence.
          m_rxEscape = true;
        } else {
          // Normal read.
          *(buf++) = data;
          blen--;
          count++;
        }
      }
      return count;
    }
    inline uint8_t activeRxCable(void) { return m_rxCable; }
    inline void send(uint8_t msgType, uint8_t data1, uint8_t data2, uint8_t channel, uint8_t cable = 0)
    {
      if (!m_running)
        return;
      if (msgType < 0xf0) {
        msgType &= 0xf0;
        if (msgType < 0x80)
          return; // Not a valid MIDI message type
        channel &= 0x0f;
        data1 &= 0x7f;
        data2 &= 0x7f;
        uint8_t msgStatus = msgType | channel;
        if (!m_runningStatus || (msgStatus != m_txRunningStatus) || (cable != m_txCable)) {
          // Cannot continue running status (off, changed status or changed cable).
          m_txRunningStatus = msgStatus;
          write(msgStatus);
        }
        write(data1);
        if ((msgType != 0xc0) && (msgType != 0xd0)) {
          write(data2);
        }
        if (m_cbSetLed) {
          m_cbSetLed(HIGH);
        }
      } else {
        if ((msgType <= 0xf8) || (cable != m_txCable)) {
          // Cancel running status.
          m_txRunningStatus = 0;
        }
        // Handle the status byte only for system common or real time.  The rest is up to the invoker.
        // TODO - implement support for more messgae types, including SYSX.
        write(msgType);
      }
    }
    inline void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0, uint8_t cable = 0) { send( 0x90, note, velocity, channel, cable ); }
    inline void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0, uint8_t cable = 0)
    {
      if (velocity != 0)
        send( 0x80, note, velocity, channel, cable );
      else
        noteOn( note, 0, channel, cable );
    }
    inline void activeSense(uint8_t cable = 0) { send( 0xfe, 0, 0, 0, cable ); if (m_cbSetLed) m_cbSetLed(HIGH); }
    inline void ctrlCh(uint8_t ccNum, uint8_t ccVal, uint8_t channel = 0, uint8_t cable = 0) { send( 0xb0, ccNum, ccVal, channel, cable ); }
    inline void progCh(uint8_t pcNum, uint8_t channel = 0, uint8_t cable = 0) { send( 0xc0, pcNum, 0, channel, cable ); }
    inline void pitchBend(uint16_t pbVal, uint8_t channel = 0, uint8_t cable = 0) { send( 0xe0, pbVal & 0x7f, (pbVal >> 7) & 0x7f, channel, cable ); }
    inline bool receiveScan(unsigned int polls = 10, uint8_t channel = 0, uint16_t cableMask = 0xffff)
    {
      uint8_t data = 0;
      while (read(&data, sizeof(data), cableMask) && --polls) {
        if (parseMidi(data, channel))
          return true;
      }
      return false;
    }
    inline void receiveFlush(uint16_t cableMask = 0)
    {
      uint8_t data = 0;
      while (read(&data, sizeof(data), cableMask)) {
      }
    }
};

#endif

