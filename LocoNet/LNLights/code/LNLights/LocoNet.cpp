// Functions for handling all the LocoNet stuff...

#define LOCONET_CPP

#include <Arduino.h>
#include <LocoNet.h>
#include "LocoNet.hpp"
#include "config.h"
#include "globals.h"
#include "eemap.h"
#include "eeprom.hpp"
#include "lnaddr.h"

// LocoNet Addresses:
// Base Address = Master On/Off
// Base + 1     = Follow LocoNet Master Power
// Base + 2     = Clock / Static mode
// Base + 3     = Fixed Day / Night
// Base + 4     = Direct toggle vs. sunrise/sunset transition
// Base + 5-8   = Begin/End storm mode (per Zone)
//------------------------------------------------------------|
// Address    | Mode                  | Closed  | Thrown      |
//************************************************************|
// Base       | Master Power          | Off     | On          |
//------------------------------------------------------------|
// Base + 1   | Follow LocoNet Power  | Static  | Fast Clock  |
//------------------------------------------------------------|
// Base + 2   | Clock Mode            | Static  | Fast Clock  |
//------------------------------------------------------------|
// Base + 3   | Day/Night             | Day     | Night       |
//------------------------------------------------------------|
// Base + 4   | Transition            | No      | Yes         |
//------------------------------------------------------------|
// Base + 5-8 | Storm Mode            | Off     | On          |
//------------------------------------------------------------|


void sendLnCVReply(lnMsg *Packet, byte status, byte data);
void write_cv(unsigned int cv, unsigned int data);
byte read_cv(unsigned int cv);
void sendTXtoLN2Byte(byte, byte);

bool addressMatch(unsigned int a) {
  if (a >= base_address && a <= base_address + 8) {
    return (true);
  } else {
    return (false);
  }
}

void handleLocoNetInterface(lnMsg *Packet) {
  unsigned int cv, data, address;
  byte status, cv_val;

  // Check for any received LocoNet packets (only if not using the Serial emulation)
  // If serial emulation, then the Serial code will set Packet != NULL
  if ( Packet )
  {
    //printRXpacket(Packet);
    // Decode the address... A0-A6 is lower 7 bits of data[1]. A7-A10 are lower 4 bits of data[2]
    address_received = decodeLnAddress(Packet);
    //Serial.println("address_received = " + String(address_received));
    if ((!addressMatch(address_received)) && (Packet->data[0] != 0xEF)) {
      //Serial.print(F("Not ours. Ignoring. A =")); Serial.println(address_received);
      return;
    }
    Serial.println(F("--------"));

    bool thrown = ((Packet->data[2] & 0x20) > 0) ? false : true; // 1=closed 0=thrown
    switch (Packet->data[0]) {
      case 0x83: // Global Power ON
        if (follow_ln) {
          setPower(true);
        }
        break;

      case 0x82: // Global Power ON
        if (follow_ln) {
          setPower(false);
        }
        break;

      case 0xBD: // OPC_SW_ACK
      case 0xB0: // OPC_SW_REQ

        // D[1] = <0, A6, A5, A4, A3, A2, A1, A0>
        // D[2] = <0, 0, DIR, ON, A10, A9, A8, A7>
        // D[2].4 (0x10) is ON/OFF (1/0)
        // D[2].5 (0x20) is CLOSE/THROW (1/0)
        // Normal operation.  Handle the request.
        printRXpacket(Packet);
        Serial.println(F("Received 0xB0 or BD : Handle lighting event"));
        // If 0xBD then send feedback. If 0xB0, do not.
        switch (address_received - base_address) {
          case 0:
            setPower(thrown);
            break;
          case 1:
            setFollowLNPower(thrown);
            break;
          case 2:
            setClockMode(thrown);
            break;
          case 3:
            setDayNightMode(thrown);
            break;
          case 4:
            setTransitionMode(thrown);
            break;
          case 5:
          case 6:
          case 7:
          case 8:
            setStormMode(address_received - base_address - 4, thrown);
            break;
          default:
            break;
        }

        // Handle ACK if needed
        if (Packet->data[0] == 0xBD) {
          // This is a fixed acknowledge response to OPC_SW_ACK (0xBD)
          //sendTXtoLN(0xB4, 0x3D, 7F);
          sendTXtoLN2Byte(0xB4, 0x3D);
        } else {
          byte d2 = (Packet->data[2] & 0x0F) | (thrown ? 0x10 : 0x20);
          sendTXtoLN2Byte(0xB4, 0x30);
          sendTXtoLN(0xB1, Packet->data[1], d2);
        }
        break;

      case 0xBC: // OPC_SW_STATE
        // Respond with current state of switch.
        // Presumably data[1]/data[2] interpreted same as 0xB0/0xBD
        break;


      case 0xEF: // OPC_WR_SL_DATA
      case 0xE7: // OPC_SL_RD_DATA
        // 0xEF 0x0E 0x7B ... fast clock update.
        Serial.println(F("Received OP_WR_SL_DATA (Fast clock)"));
        printRXpacket(Packet);
        //if (Packet->data[1] == 0x0E && Packet->data[2] == 0x7B) {
        if (Packet->fc.slot == 0x7B && Packet->fc.clk_cntrl & 0x40) {
          update_fast_clock(Packet);
        } else {
          // 0xEF 0x0E 0x7C <PCMD><0><HOPSA><LOPSA><TRK><CVH><CVL><DATA7><0><0><CHK>
          // Write to programming track.  First, check if this is our address...
          //address = (Packet->data[5] << 7) + Packet->data[6];
          //Serial.println("0xEF Address = " + String(address) + "(decoder = " + String(decoder_cv_address) + ")");
        }
        break;

      default:
        break;
    } // end switch (opcode)

  } // endif Packet

}

void update_fast_clock(lnMsg *msg) {
  // D[1] = 0x0E
  // D[2] = 0x7B
  // D[3] = <CLK_RATE> 0=freeze Nonzero = N:1 rate up to 128:1 (7F)
  // D[4] = <FRAC_MINSL> fractions of a minute, low byte
  // D[5] = <FRAC_MINSH> fractions of a minute, high byte
  // D[6] = <256-MINS_60> Fast clock minutes subtracted from 256, mod 60
  // D[7] = <TRK>
  // D[8] = <256-HRS_24> Fast clock hours sbtracted from 256, mod 24
  // D[9] = <DAYS> Number of 24-hour rolls, positive count
  // D[10] = <CLK_CNTRL> Clock control byte: 1=valid 0=ignore
  // D[11]/D[12] = <ID1><ID2> device ID of last clock setter <00><00> is no set. <7F><7x> reserved

  Serial.println(F("Updating fast clock"));
  fastClockMsg *fcMsg = (fastClockMsg *)msg;

  //if (fcMsg->clk_cntrl == 0) return; // Ignore if the clock control bit is 0

  fast_clock.rate = fcMsg->clk_rate;
  //fast_clock.frac = fcMsg->frac_minsh * 256 + fcMsg->frac_minsl;
  fast_clock.frac = 0x3FFF - ((fcMsg->frac_minsh << 7) + fcMsg->frac_minsl);
  //fast_clock.mins = (256 - fcMsg->mins_60) % 60;  
  fast_clock.mins = fcMsg->mins_60 - (127-60);
  //fast_clock.hours = (256 - fcMsg->hours_24) % 24;
  fast_clock.hours = (fcMsg->hours_24 >= (128-24)) ? fcMsg->hours_24 - (128-24) : fcMsg->hours_24 % 24;
  fast_clock.days = fcMsg->days;
  Serial.print(F("rate=")); Serial.println(fast_clock.rate);
  Serial.print(F("time=")); Serial.print(fast_clock.hours); Serial.print(F(":"));
  Serial.print(fast_clock.mins); Serial.print(F(":")); Serial.println(fast_clock.frac);
  if (clock_mode == CLOCK_MODE_FASTCLOCK) {
    updateCurrentTime(true);
  }
}

unsigned int decodeLnAddress(lnMsg *msg) {
  /*
  // DEBUG
  Serial.print("decode data[1] = " + String(msg->data[1]));
  Serial.print(" data[2] = " + String(msg->data[2]));
  Serial.print(" d1 = " + String(msg->data[1] & 0x7F));
  Serial.println(" d2 = " + String(msg->data[2] & 0x0F));
  Serial.println("Final addr = " + String((msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 7)));
*/
  // This is for received commands (e.g. Turnouts 0xB0, 0xBD)
  return ((msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 7) + 1);
  // This is for sent messages (e.g. Sensors 0xB2)
  //return(msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 8) + (( msg->data[2] & 0x20) ? 1 : 0);
}

void printRXpacket (lnMsg *packet)
{
#if (SERIAL_EMULATE_LN && 0)
  uint8_t Length = 4;
#else
  uint8_t Length = getLnMsgSize( packet ) ;
#endif
  Serial.print(F("RX: "));
  for ( uint8_t Index = 0; Index < Length; Index++ )
  {
    Serial.print(packet->data[ Index ], HEX);
    Serial.print(F("  "));
  }
  Serial.println();
}

/*
void sendLnCVReply(lnMsg *Packet, byte status, byte data) {
  // <CVH> includes Data bit 7 in its bit 2.
  // <DATA7> includes only bits 6-0 of the data
  byte cvh = ((Packet->data[8] & 0xFE) | ((data & 0x80) >> 6));
  byte checksum = (0xE7 ^ 0x0E ^ 0x7C ^ Packet->data[3] ^ status ^
                   Packet->data[5] ^ Packet->data[6] ^ Packet->data[7] ^ cvh ^
                   Packet->data[9] ^ (data & 0x7F) ^ 0x00 ^ 0x00);

  uint8_t start = LnTxBuffer.WriteIndex;
  LnPacket->data[0] = 0xE7;
  LnPacket->data[1] = 0x0E;
  LnPacket->data[2] = 0x7C;
  LnPacket->data[3] = Packet->data[3];
  LnPacket->data[4] = status;
  LnPacket->data[5] = Packet->data[5];
  LnPacket->data[6] = Packet->data[6];
  LnPacket->data[7] = Packet->data[7];
  LnPacket->data[8] = cvh;
  LnPacket->data[9] = Packet->data[9];
  LnPacket->data[10] = (data & 0x7F);
  LnPacket->data[11] = Packet->data[11];
  LnPacket->data[12] = Packet->data[12];
  LnPacket->data[13] = 0xFF - checksum;

  // Check to see if we have a complete packet...
  //LnPacket = recvLnMsg( &LnTxBuffer ) ;
  if (LnPacket )
  {
    // Send the packet to the LocoNet
    LocoNet.send( LnPacket );
    Serial.print(F("TX: "));
    for (int i = 0; i < 14; i++) {
      Serial.print(LnPacket->data[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F(" "));
  }
  else {
    Serial.println(F("Bad packet..."));
    Serial.print(F("TX: "));
    for (int i = 0; i < 14; i++) {
      Serial.print(LnPacket->data[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F(" "));
  }
}
*/

void sendTXtoLN (byte opcode, byte firstbyte, byte secondbyte)
{
  byte checksum = 0xFF - (opcode ^ firstbyte ^ secondbyte);

  // Add bytes to the buffer
  addByteLnBuf( &LnTxBuffer, opcode ) ;
  addByteLnBuf( &LnTxBuffer, firstbyte ) ;
  addByteLnBuf( &LnTxBuffer, secondbyte ) ;
  addByteLnBuf( &LnTxBuffer, checksum ) ;

  // Check to see if we have a complete packet...
  LnPacket = recvLnMsg( &LnTxBuffer ) ;
  if (LnPacket )
  {
    // Send the packet to the LocoNet
    LocoNet.send( LnPacket );
    Serial.print(F("TX: "));
    Serial.print(opcode, HEX);
    Serial.print(F("  "));
    Serial.print(firstbyte, HEX);
    Serial.print(F("  "));
    Serial.print(secondbyte, HEX);
    Serial.print(F("  "));
    Serial.print(checksum, HEX);
    Serial.println();

  }
}

void sendTXtoLN2Byte (byte opcode, byte firstbyte)
{
  byte checksum = 0xFF - (opcode ^ firstbyte);

  // Add bytes to the buffer
  addByteLnBuf( &LnTxBuffer, opcode ) ;
  addByteLnBuf( &LnTxBuffer, firstbyte ) ;
  addByteLnBuf( &LnTxBuffer, checksum ) ;

  // Check to see if we have a complete packet...
  LnPacket = recvLnMsg( &LnTxBuffer ) ;
  if (LnPacket )
  {
    // Send the packet to the LocoNet
    LocoNet.send( LnPacket );
    Serial.print(F("TX: "));
    Serial.print(opcode, HEX);
    Serial.print(F("  "));
    Serial.print(firstbyte, HEX);
    Serial.print(F("  "));
    Serial.print(checksum, HEX);
    Serial.println();

  }
}

/*
  bool addressIsMine(unsigned int addr) {
  //return((addr >= base_address) && (addr < base_address + NUM_SIGNAL_SUBADDRS + NUM_SERVOS + NUM_LOCKS));
  return(true);
  }
*/

/*
void write_cv(unsigned int cv, unsigned int data) {
  Serial.println("Writing to CV: " + String(cv) + " Val: " + String(data));
  switch (cv) {
    case CV_DECODER_ADDRESS_LSB:
      writeCVToEEPROM(EEPROM_CV_DECODER_ADDRESS_LSB, data); break;
    case CV_DECODER_ADDRESS_MSB:
      writeCVToEEPROM(EEPROM_CV_DECODER_ADDRESS_MSB, data); break;
    //case CV_ACC_DECODER_CONFIG:
    //writeCVToEEPROM(EEPROM_CV29, data); break;
    case CV_HEAD1_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_HEAD1_FUNCTION, data); break;
    case CV_HEAD2_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_HEAD2_FUNCTION, data); break;
    case CV_HEAD3_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_HEAD3_FUNCTION, data); break;
    case CV_HEAD4_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_HEAD4_FUNCTION, data); break;
    case CV_SERVO1_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_SERVO1_FUNCTION, data); break;
    case CV_SERVO2_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_SERVO2_FUNCTION, data); break;
    case CV_SERVO3_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_SERVO3_FUNCTION, data); break;
    case CV_SERVO4_FUNCTION:
      writeCVToEEPROM(EEPROM_CV_SERVO4_FUNCTION, data); break;
    default:
      break;
  }
}

byte read_cv(unsigned int cv) {
  byte val = 0;
  switch (cv) {
    case CV_DECODER_ADDRESS_LSB:
      val = readCVFromEEPROM(EEPROM_CV_DECODER_ADDRESS_LSB); break;
    case CV_DECODER_ADDRESS_MSB:
      val = readCVFromEEPROM(EEPROM_CV_DECODER_ADDRESS_MSB); break;
    case CV_ACC_DECODER_CONFIG:
      val = 0x88; //CV29 is read only, fixed value in this case 
    case CV_HEAD1_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_HEAD1_FUNCTION); break;
    case CV_HEAD2_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_HEAD2_FUNCTION); break;
    case CV_HEAD3_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_HEAD3_FUNCTION); break;
    case CV_HEAD4_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_HEAD4_FUNCTION); break;
    case CV_SERVO1_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_SERVO1_FUNCTION); break;
    case CV_SERVO2_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_SERVO2_FUNCTION); break;
    case CV_SERVO3_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_SERVO3_FUNCTION); break;
    case CV_SERVO4_FUNCTION:
      val = readCVFromEEPROM(EEPROM_CV_SERVO4_FUNCTION); break;
    default:
      break;
  }
  Serial.print(F("Reading CV: "));
  Serial.print(cv);
  Serial.print(F(" Val: "));
  Serial.print(val);
  return (val);
}
*/

