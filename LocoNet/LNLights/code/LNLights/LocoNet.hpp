extern void handleLocoNetInterface(lnMsg *Packet);
extern unsigned int decodeLnAddress(lnMsg *msg);
extern void printRXpacket(lnMsg *packet);
extern void sendTXtoLN(byte opcode, byte firstbyte, byte secondbyte);
//extern bool addressIsMine(unsigned int addr);
//extern void reportSensorState(byte sensor, bool state);
//extern void reportTurnoutState(unsigned int addr, bool state);
extern void update_fast_clock(lnMsg *msg);
