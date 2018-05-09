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

unsigned int address_received;
//unsigned int decoder_cv_address;

void sendLnCVReply(lnMsg *Packet, byte status, byte data);
void write_cv(unsigned int cv, unsigned int data);
byte read_cv(unsigned int cv);

bool addressMatch(unsigned int a) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    if ((Servos[i].address() == a) || (Servos[i].lockAddress() == a)) {
      return(true);
    }
  }
  return(false);
}

bool addressIsServo(unsigned int a) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (Servos[i].address() == a) {
      Serial.print(F("Servo found: a = ")); Serial.print(a); Serial.print(F(" Servo = ")); Serial.print(i);
      Serial.print(F(" SA = ")); Serial.println(Servos[i].address());
      return(true);
    }
  }
  return(false);
}

bool addressIsLock(unsigned int a) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (Servos[i].lockAddress() == a) {
      Serial.print(F("Lock found: a = ")); Serial.print(a); Serial.print(F(" Lock = ")); Serial.print(i);
      Serial.print(F(" LA = ")); Serial.print(Servos[i].lockAddress());
      return(true);
    }
  }
  return(false);
}

void handleLocoNetInterface(lnMsg *Packet) {
  unsigned int cv, data, address;
  byte status, cv_val;
  
  // Check for any received LocoNet packets (only if not using the Serial emulation)
  // If serial emulation, then the Serial code will set Packet != NULL
  if( Packet )
  {
    //printRXpacket(Packet);
    // Decode the address... A0-A6 is lower 7 bits of data[1]. A7-A10 are lower 4 bits of data[2]
    address_received = decodeLnAddress(Packet);
    //Serial.println("address_received = " + String(address_received));
    if (!addressMatch(address_received)) {
      //Serial.println("Not ours. Ignoring.");
      return;
    }

    switch(Packet->data[0]) {
      case 0x83: // Global Power ON
        //sendALLsensors(); // does this really do anything?
        break;

      case 0xBD: // OPC_SW_ACK  
      case 0xB0: // OPC_SW_REQ
          // D[1] = <0, A6, A5, A4, A3, A2, A1, A0>
          // D[2] = <0, 0, DIR, ON, A10, A9, A8, A7>
          // D[2].4 (0x10) is ON/OFF (1/0)
          // D[2].5 (0x20) is CLOSE/THROW (1/0)
            // Normal operation.  Handle the request.
            printRXpacket(Packet);
            Serial.println(F("State 0 - Received 0xB0 or BD : Handle signal or servo change"));
            // If 0xBD then send feedback. If 0xB0, do not.
            if ( addressIsServo(address_received) && ((Packet->data[2] & 0x10) > 0)) {
              Serial.print(F("Handling Servo Change: address = ")); Serial.println(address_received);
              handleLNServoChange(address_received, Packet->data[2], (Packet->data[0] == 0xBD));
            } else if (addressIsLock(address_received) && ((Packet->data[2] & 0x10) > 0) ) {
              Serial.print(F("Handling Lock Change: address = ")); Serial.println(address_received);
              handleLockChange(address_received, Packet->data[2], (Packet->data[0] == 0xBD));
            } else {
              if (((Packet->data[2] & 0x10) > 0)) {
                Serial.println(F("Strange.  Bit 5 set."));
              } else {
                Serial.println(F("Bit 5 not set."));
              }
              ; // do nothing... no signals!
              Serial.println(F("Nothing to do."));
            }

            // Handle ACK if needed
            if (Packet->data[0] == 0xBD) {
              // This is a fixed acknowledge response to OPC_SW_ACK (0xBD)
              sendTXtoLN (0xB4, 0x3D, 0x7F);
            }
            break;

      case 0xBC: // OPC_SW_STATE
        // Respond with current state of switch.
        // Presumably data[1]/data[2] interpreted same as 0xB0/0xBD
        break;
            
              
      case 0xEF: // OPC_WR_SL_DATA -- not supported just yet
/*
        // 0xEF 0x0E 0x7C <PCMD><0><HOPSA><LOPSA><TRK><CVH><CVL><DATA7><0><0><CHK>
        // Write to programming track.  First, check if this is our address...
        printRXpacket(Packet);
        address = (Packet->data[5] << 7) + Packet->data[6];
        Serial.println("0xEF Address = " + String(address) + "(decoder = " + String(decoder_cv_address) + ")");
        if ((address == 0) || (address == decoder_cv_address)) {
          // Immediately ack the message
          sendTXtoLN(0xB4, 0x7F, 0x01); // Ack "Task accepted"

          // Handle the command.
          switch(Packet->data[3]) {
            case 0x28: // Direct mode byte read on service track
            case 0x2B:
              // Construct the CV number
              // Bits 8-9 of CV are bits 4-5 of data[8].  Bit 7 of CV is bit 0 of data[8]. Bits 6-0 of CV are data[9]
              cv = ((Packet->data[8] & 0x30) << 4) + ((Packet->data[8] & 0x01) << 7) + Packet->data[9]; 

              cv_val = read_cv(cv);
              Serial.println("CV Read: CV=" + String(cv) + " Val=" + String(cv_val));
              status = 0x00; // TODO: 0 is success. Set error codes if needed
              sendLnCVReply(Packet, status, cv_val);
              break;
              
            case 0x68: // Direct mode byte write on service track
            case 0x6B:
            case 0x67: // Ops byte write
            //case 0x60: // Paged mode byte write on service track -- only supporting direct mode write for now.
              // Construct the CV number
              // Bits 8-9 of CV are bits 4-5 of data[8].  Bit 7 of CV is bit 0 of data[8]. Bits 6-0 of CV are data[9]
              cv = ((Packet->data[8] & 0x30) << 4) + ((Packet->data[8] & 0x01) << 7) + Packet->data[9]; 
              // Construct the data
              // Data bit 7 is bit 2 of data[8].  Data bits 6-0 are data[10]
              data = ((Packet->data[8] & 0x20) << 6) + Packet->data[10];
              
              Serial.println("CV Write: CV=" + String(cv) + " Val=" + String(data));
              write_cv(cv, data);
              
              // Send reply
              // 0xE7 0x0E 0x7C <PCMD><PSTAT><HOPSA><LOPSA><TRK><CVH><CVL><DATA7><0><0><CHK>
              unsigned char status = 0x00; // TODO: 0 is success. Set error codes if needed
              sendLnCVReply(Packet, status, data);              
              break;
          }
        }
        */
        break;
      
      default:
        break;      
    } // end switch (opcode)

  } // endif Packet

}

unsigned int decodeLnAddress(lnMsg *msg) {
  /*
  Serial.print(F("decode data[1] = ")); Serial.print(msg->data[1]);
  Serial.print(F(" data[2] = ")); Serial.print(msg->data[2]);
  Serial.print(F(" d1 = ")); Serial.print(msg->data[1] & 0x7F);
  Serial.print(F(" d2 = ")); Serial.print(msg->data[2] & 0x0F);
  Serial.print(F("Final addr = ")); Serial.print((msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 7));
  */
  return((msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 7) + 1);
}

void printRXpacket (lnMsg *packet)
{
#if (SERIAL_EMULATE_LN && 0)
    uint8_t Length = 4;
#else
    uint8_t Length = getLnMsgSize( packet ) ;  
#endif
    Serial.print(F("RX: "));
    for( uint8_t Index = 0; Index < Length; Index++ )
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
  */
/*    
    addByteLnBuf( &LnTxBuffer, 0xE7 ) ;
    addByteLnBuf( &LnTxBuffer, 0x0E ) ;
    addByteLnBuf( &LnTxBuffer, 0x7C ) ;
    addByteLnBuf( &LnTxBuffer, Packet->data[3] ) ; // <PCMD>
    addByteLnBuf( &LnTxBuffer, status) ;
    addByteLnBuf( &LnTxBuffer, Packet->data[5] ) ; // <HOPSA>
    addByteLnBuf( &LnTxBuffer, Packet->data[6] ) ; // <LOPSA>
    addByteLnBuf( &LnTxBuffer, Packet->data[7] ) ; // <TRK>
    addByteLnBuf( &LnTxBuffer, cvh ) ; // <CVH>
    addByteLnBuf( &LnTxBuffer, Packet->data[9] ) ; // <CVL>
    addByteLnBuf( &LnTxBuffer, (data & 0x7F) ) ; // <DATA7>
    addByteLnBuf( &LnTxBuffer, Packet->data[11] ) ;
    addByteLnBuf( &LnTxBuffer, Packet->data[12] ) ;
    addByteLnBuf( &LnTxBuffer, 0xFF - checksum ) ;
*/
/*
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
    LnPacket->data[13] = 0xFF-checksum;

    // Check to see if we have a complete packet...
    //LnPacket = recvLnMsg( &LnTxBuffer ) ;
    if(LnPacket )
    {
        // Send the packet to the LocoNet
      LocoNet.send( LnPacket );
      Serial.print("TX: ");
      for (int i = 0; i < 14; i++) {
        Serial.print(LnPacket->data[i], HEX);
        Serial.print(" ");
      }
      Serial.println(" ");
     }
     else {
      Serial.println("Bad packet...");
      Serial.print("TX: ");
      for (int i = 0; i < 14; i++) {
        Serial.print(LnPacket->data[i], HEX);
        Serial.print(" ");
      }
      Serial.println(" ");
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
    if(LnPacket )
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

/*
void write_cv(unsigned int cv, unsigned int data) {
  Serial.println("Writing to CV: " + String(cv) + " Val: " + String(data));
  switch(cv) {
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
  switch(cv) {
    case CV_DECODER_ADDRESS_LSB:
      val = readCVFromEEPROM(EEPROM_CV_DECODER_ADDRESS_LSB); break;
    case CV_DECODER_ADDRESS_MSB:
      val = readCVFromEEPROM(EEPROM_CV_DECODER_ADDRESS_MSB); break;
    case CV_ACC_DECODER_CONFIG:
      val = 0x88; // CV29 is read only, fixed value in this case 
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
  Serial.println("Reading CV: " + String(cv) + " Val: " + String(val));
  return(val);
}
*/

/*
void reportSensorState(byte sensor, bool state) {
  byte byte1, byte2;
  byte1 = ((sensor_base_address + sensor) & 0xFF) >> 1; // A7-A1
  byte2 = (sensor_base_address + sensor) / 256; // A10-A8     
  byte2 |= (state? 0x50 : 0x40); // State encoded in bit 4. Bit 6 is always high. Bit 5 is always low
  byte2 |= ((sensor & 0x01) ? 0x20 : 0x00); // A0 goes in bit 5
  Serial.println("RSS: Sensor " + String(sensor) + "state " + String(state));
  sendTXtoLN(OPC_INPUT_REP, byte1, byte2); // OPC_SW_REP -- Sensor Report    
}
*/

void reportTurnoutState(unsigned int addr, bool isThrown) {
  byte byte1, byte2;
  byte1 = (addr & 0xFF) >> 1;          // A7-A1
  byte2 = addr / 256;  // A10-A7
  byte2 |= ((addr & 0x01) ? 0x20 : 0x00); // A0 goes in bit 5
  byte2 |= (isThrown ? 0x60 : 0x70); // State encoded in bit 4. Bit 6 is always high. Bit 5 is always high
  
  Serial.print(F("Report: 0xB1, "));
  Serial.print(byte1, HEX); Serial.print(F(", "));
  Serial.println(byte2, HEX);
  sendTXtoLN(OPC_SW_REP, byte1, byte2); // OPC_SW_REP -- Sensor Report    
}

