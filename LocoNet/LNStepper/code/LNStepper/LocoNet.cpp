// Functions for handling all the LocoNet stuff...

#define LOCONET_CPP

#include <Arduino.h>
#include <LocoNet.h>
#include "LocoNet.hpp"
#include "config.h"
#include "globals.h"
#include "eemap.h"
#include "eeprom.hpp"

extern int sensorstate[NUM_SENSORS];

#define ENABLE_LOCONET 1

void handleLocoNetInterface(lnMsg *Packet) {
  // Check for any received LocoNet packets (only if not using the Serial emulation)
  // If serial emulation, then the Serial code will set Packet != NULL
  if( Packet && ENABLE_LOCONET)
  {
    address_received = decodeLnAddress(Packet);
    //if ((Packet->data[0] == OPC_INPUT_REP) && addressIsMine(address_received)) {
      // I just sent this.  Ignore.
      //return;
    //}
    printRXpacket(Packet);
    // Decode the address... A0-A6 is lower 7 bits of data[1]. A7-A10 are lower 4 bits of data[2]
    Serial.println("address_received = " + String(address_received));

    switch(Packet->data[0]) {
      case 0x83: // Global Power ON
        //sendALLsensors(); // does this really do anything?
        break;

      case 0xBD: // OPC_SW_ACK  
      case 0xB0: // OPC_SW_REQ
        switch(stato) {
          case 0:
            // Normal operation.  Handle the request.
            Serial.println("Stato 0 - Received 0xB0 or BD : Handle signal or servo change");
            // If 0xBD then send feedback. If 0xB0, do not.
            if (address_received == sensor1_address) {
                reportSensorState(0, sensorstate[0] == 1);
            } else if (address_received == sensor2_address) {
                reportSensorState(1, sensorstate[1] == 1);
            } else if (address_received == sensor3_address) {
                reportSensorState(2, sensorstate[2] == 1);
            } else if (address_received == sensor4_address) {
                reportSensorState(3, sensorstate[3] == 1); 
            } else if (address_received == sensor5_address) {
                reportSensorState(4, sensorstate[4] == 1);
            } else if (address_received == sensor6_address) {
                reportSensorState(5, sensorstate[5] == 1);
            } else {
                // Do nothing.
            }
            break;
            
          case 1:        
            // State 1: Store address of this device.
            Serial.println("Stato 1 - Received B0: Store my address");
            base_address = address_received;
            writeAddressToEEPROM(EEPROM_BASE_ADDRESS, base_address);
            Serial.print("\t");
            Serial.print(base_address, HEX);
            Serial.println();
       
            Serial.println("...move to state 2");  
            stato = 2;
            lamp = 200;
            break;       
       
          case 2:
            // State 2: Store broadcast address.
            Serial.println("Stato 2 - Received B0: Store broadcast address");
            // TODO: Fix this. Broadcast address is more complex than this.
            //
            broadcast = address_received;
            writeAddressToEEPROM(EEPROM_BC_ADDRESS, broadcast);
            
            Serial.print("\t");
            Serial.print(broadcast, HEX);
            Serial.println();  
      
            Serial.println("fatto! return to stato 0");  
            stato = 0;
            lamp = 400;
            digitalWrite(led, LOW);
            break;
        } 
        break;      
            
      default:
        break;      
    } // end switch (opcode)
#if (SERIAL_EMULATE_LN > 0)
    //Packet = NULL;
#endif
  } // endif Packet

}

unsigned int decodeLnAddress(lnMsg *msg) {
  Serial.print("decode data[1] = " + String(msg->data[1]));
  Serial.print(" data[2] = " + String(msg->data[2]));
  Serial.print(" d1 = " + String(msg->data[1] & 0x7F));
  Serial.println(" d2 = " + String(msg->data[2] & 0x0F));
  Serial.println("Final addr = " + String((msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 7)));
  return((msg->data[1] & 0x7F) + ((msg->data[2] & 0x0F) << 7));
}

void printRXpacket (lnMsg *packet)
{
#if (SERIAL_EMULATE_LN && 0)
    uint8_t Length = 4;
#else
    uint8_t Length = getLnMsgSize( packet ) ;  
#endif
    Serial.print("RX: ");
    for( uint8_t Index = 0; Index < Length; Index++ )
    {
      Serial.print(packet->data[ Index ], HEX);
      Serial.print("  ");
     } 
    Serial.println();
}

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
      Serial.print("TX: ");
      Serial.print(opcode, HEX);
      Serial.print("  ");
      Serial.print(firstbyte, HEX);
      Serial.print("  ");
      Serial.print(secondbyte, HEX);
      Serial.print("  ");
      Serial.print(checksum, HEX);
      Serial.println();

     }
}

bool addressIsMine(unsigned int addr) {
  return((addr >= base_address) && (addr < base_address + NUM_SENSOR_SUBADDRS));
}

void reportSensorState(byte sensor, bool state) {
  byte byte1, byte2;
  // Weird Digitrax DS52 style addressing.
  // Byte1 is (effectively) 0 A7 A6    A5  A4 A3 A2 A1
  // Byte2 is (effectively) 0  1 A0 State 0 A10 A9 A8
  //byte1 = ((base_address + sensor) >>1) & 0x7F; // A6-A0
  Serial.println("Base+sensor " + String(base_address+sensor));
  byte1 = ((base_address + sensor - 1) & 0xFF) >> 1; // A7-A1
  byte2 = ((base_address + sensor)) / 256;
  byte2 |= (state ? 0x50 : 0x40);                // State encoded in bit 4. Bit 6 is always high. Bit 5 is always low
  byte2 |= ((sensor & 0x01) ? 0x20 : 0x00); //  A0 goes in bit 5 
  sendTXtoLN(OPC_INPUT_REP, byte1, byte2); // OPC_SW_REP -- Sensor Report    
}




