#include <mcp_can.h>
#include <SPI.h>

// Variables
  unsigned long stamp100ms = 0;
  unsigned long stamp200ms = 0;
  unsigned long stamp500ms = 0;
  byte ABScount = 0;
  byte ABS2count = 0xB3;
  byte ABS3count = 0;
  byte ABGcount = 0;

MCP_CAN CAN0(9); // Set CS pin of CAN-Bus shield

void setup() {
  Serial.begin(115200);
  // Initialize MCP2515 running at 16MHz with a baudrate of 100kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_100KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");
  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted

  // Variables init
  stamp100ms = millis();
  stamp200ms = millis();
  stamp500ms = millis();
}

// Simplified command for sending frames
void canSend(unsigned short id=0, byte l=0, byte a=0, byte b=0, byte c=0, byte d=0, byte e=0, byte f=0, byte g=0, byte h=0){
  byte data[8]{a, b, c, d, e, f, g, h};
  CAN0.sendMsgBuf(id, 0, l, data);
}

// Ignition
byte igncount = 0x00;
byte ign;
void ignition(bool state = 0){ // Ignition
  switch(state){
    case 1:
      ign = 0x45;
      break;
    case 0:
      ign = 0x00;
      break;
  }
  canSend(0x130, 6, ign, 0x40, 0x21, 0x8F, igncount);
  igncount++;
}

// Coolant + Oil temperature
byte tempcount = 0x00;
void oilTemp(short t = 0){ // Celsius
  canSend(0x1D0, 8, 0x93, t+48, tempcount, 0xCD, 0x5D, 0x37, 0x0D, 0x84); // Coolant temp is on B0, which is irrelevant
  tempcount++;
}

// Engine speed
void RPM(unsigned short rev = 0){ // RPM
  unsigned short RPMsend = rev * 4; 
  canSend(0x0AA, 8, 0x5F, 0x59, 0xFF, 0x00, lowByte(RPMsend), highByte(RPMsend), 0x80, 0x99);
}

// Lights
byte lightVal = 0; // bit 1 = highbeam, bit 2 = backlight
void light(bool lo = 1, bool hi = 0){
  bitWrite(lightVal, 2, lo); // Sets backlight
  bitWrite(lightVal, 1, hi); // Sets highbeam
  canSend(0x21A, 3, lightVal, 0x00, 0xF7);
}

// Indicators
byte indicatorVal = 0x80; // 0x80 = silent, 0x81 = acoustic clicking, 0x82 = fast acoucstic clicking, 0x83 = alternating fast acoustic clicking
void indicators(bool l = 0, bool r = 0){
  bitWrite(indicatorVal, 4, l);
  bitWrite(indicatorVal, 5, r);
  canSend(0x1F6, 2, indicatorVal, 0xF0);
}

// Handbrake
void handbrake(bool status = 0){
  canSend(0x34F, 2, 0xFD + status, 0xFF);
} 

// Gears
byte gcount = 0x0C;
byte Gearmode = 0xE1;
byte Gearsport = 0xF0;
byte Gear = 0x0F;
void gear(char g = 'P'){
  switch(g){
    case 'P':
      gcount = gcount | 0x0C;
      Gearmode = 0xE1;
      Gearsport = 0xF0;
      Gear = 0x0F;
      break;
    case 'R':
      gcount = gcount | 0x0C;
      Gearmode = 0xD2;
      Gearsport = 0xF0;
      Gear = 0x0F;
      break;
    case 'N':
      gcount = gcount | 0x0D;
      Gearmode = 0xB4;
      Gearsport = 0xF0;
      Gear = 0x0F;
      break;
    case 'D':
      gcount = gcount | 0x0D;
      Gearmode = 0x78;
      Gearsport = 0xF0;
      Gear = 0x0F;
      break;
    default:
      gcount = gcount | 0x0D;
      Gearmode = 0x78;
      Gearsport = 0xF2;
      if(g > 8){g = 8;}
      Gear = 0x4F + g * 0x10;
      break;
  }
  
  gcount+=0x10;
  canSend(0x1D2, 6, Gearmode, Gear, 0xFF, gcount, Gearsport, 0xFF, 0x00, 0x00);
}
// Time
void time(byte h = 4, byte m = 20, byte s = 0){ // hours, minutes, seconds
  canSend(0x39E, 8, h, m, s, 0x01, 0x1F, 0xDF, 0x0F, 0xF2);
}

// Gimme Fuel
void fuel(unsigned short V = 0){ // Liters
  canSend(0x349, 5, lowByte(V*160), highByte(V*160), lowByte(V*160), highByte(V*160), 0x00);
}

// Check-Control
void check(unsigned short id = 0, bool state = 1){ // CC-ID
  if(id == 0){state = 0;}
  canSend(0x592, 8, 0x40, lowByte(id), highByte(id), state, 0xFF, 0xFF, 0xFF, 0xFF);
}


// Speed
unsigned short lastSpeedVal = 0;
unsigned short speedCount = 0;
void speed(unsigned short v = 0) { // km/h
  unsigned short speedVal = v/1.5 + lastSpeedVal;
  canSend(0x1A6, 8, lowByte(speedVal), highByte(speedVal), lowByte(speedVal), highByte(speedVal), lowByte(speedVal), highByte(speedVal), lowByte(speedCount), highByte(speedCount) | 0xF0);
  speedCount+=200;
  lastSpeedVal = speedVal;
}

// Cruise control dial
byte cccount = 1;
void cruise(unsigned short v = 0){
  canSend(0x193, 8, cccount, (v+1)/1.6, 0xF5, 0x00, 0xF8, 0x50, 0x01, 0x00);
  cccount*=17;
}

void loop() {
  if(stamp100ms + 100 <= millis()){ // Run every 100ms
    ignition(1);
    handbrake(0);
    speed(0); // km/h
    RPM(800); // RPM

    // Various errors
    canSend(0x19E, 8, 0x00, 0xE0, ABS2count, 0xFC, 0xF0, 0x43, 0x00, ABS3count); // ABS, handbrake, TPMS fault
    ABS2count = ((((ABS2count >> 4) + 3) << 4) & 0xF0) | 0x03;
    ABS3count++;
    canSend(0x0C1, 2, random(0,255), 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00); //Seems to clear sos error
    stamp100ms = millis();
  }  

  if(stamp200ms + 200 <= millis()){ // Run every 200ms
    gear('P'); // 'P', 'R', 'N', 'D' or 1, 2, 3, 4, 5, 6, 7, 8
    oilTemp(100); // °Celsius
    light(1, 0); // Backlight, Highbeam
    indicators(0, 0); // Left, Right
    time(16, 24); // Hours, Minutes, Seconds
    fuel(63); // Liters - full tank is 63l
    //check(0); // CC-ID, on/off
    cruise(0); // km/h

    canSend(0x26E, 8, 0x40, 0x40, 0x7F, 0x50, 0xFF, 0xFF, 0xFF, 0xFF); // Key status

    // Various errors
    canSend(0x0C0, 2, ABScount, 0xFF); // ABS, handbrake, TPMS fault
    ABScount++;
    ABScount = ABScount | 0xF0;
    canSend(0x0D7, 2, ABGcount, 0xFF);
    ABGcount++;

    stamp200ms = millis();
  }  


}