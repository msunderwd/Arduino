extern void handleLocoNetInterface(lnMsg *Packet);
extern unsigned int decodeLnAddress(lnMsg *msg);
extern void printRXpacket(lnMsg *packet);
extern void sendTXtoLN(byte opcode, byte firstbyte, byte secondbyte);
extern bool addressIsMine(unsigned int addr);
extern void reportSensorState(byte sensor, bool state);

#define OPC_INPUT_REP 0xB2
#define OPC_SW_REP    0xB1
#define OPC_LONG_ACK  0xB4

