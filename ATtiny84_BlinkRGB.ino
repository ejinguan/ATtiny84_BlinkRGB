/*
  Blink (modified slightly for ATTiny84/44 ICs
  Turns on an LED on for one second, then off for one second, repeatedly.

  This example code is in the public domain.
 */
 

// Software Serial Library
#include<SoftwareSerial.h>

#define rxPin 10
#define txPin 0

SoftwareSerial mySerial(rxPin, txPin);
byte myAddr = 0;   // Store address of this node


// Communication state variables
bool forward;      // Flag to forward current command to next node
byte cmd[6];       // Command array
int cmdLength;     // Command bytes received

byte buf[20];
int i = 0;         // general counter
int len = 0;       // buffer length (0-based)
int temp = 0;      // current integer built up
char mode = '\0';  // current mode (H/S/I)
bool serialReady = false;


// IR sensor
#define IR_INPUT A2
#define IR_LED   9

// IR state variables
int IR_val = 0;
bool IR_enabled = true;


// LEDs
// Set to true if LEDs have common 5v and sink individually
bool common_anode = true;

int pinRed   = 6;
int pinGreen = 7;
int pinBlue  = 8;

// Colour state variables
byte R = 10;
byte G = 20;
byte B = 50;
// Initial colour = Green
int H = 120; // 0-360
int S = 100; // 0-100
int I =  10; // 0-100



// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  pinMode(pinBlue, OUTPUT);

  pinMode(IR_INPUT, INPUT);
  pinMode(IR_LED, OUTPUT);

  mySerial.begin(9600);
  mySerial.println("Connected!");
}



// the loop routine runs over and over again forever:
void loop() {

  if (IR_enabled) {
    // Collect IR readings every cycle
    digitalWrite(IR_LED, HIGH);
    IR_val = 0.8*IR_val + 0.2*analogRead(IR_INPUT);
    digitalWrite(IR_LED, LOW);
  } else {
    IR_val = 0;
  }

  // Map IR readings
//  I = constrain(IR_val/2, 0, 100);
  // H = map(IR_val, 40, 400, 120, 0);
  // H = constrain(H, 0, 120);
  

  // Read serial data
  if (mySerial.available()) {
    // Empty cmd array
    for (i = 0; i < 6; i++) {
      cmd[i] = 0;
    }
    
    // Read 1st byte - Command 1
    cmd[0] = GetNextByte();

    if (cmd[0] == myAddr)  {
      // Address to myself
      // Read R G B
      R = GetNextByte();
      G = GetNextByte();
      B = GetNextByte();
      cmdLength = 4;
      
    } else if (cmd[0] == 0xFF) {
      // Address to all
      // Read 2nd byte - Command 2
      cmd[1] = GetNextByte();
      
      switch (cmd[1]) {
        case 0xAA: // Node count
          cmd[2] = GetNextByte();   // Current node count
          myAddr = cmd[2];          // This node takes this count (from 0)
          cmd[2]++;                 // Increment to prepare for sending
          cmdLength = 3;
          break;
          
        case 0x00: // All Off
          R = 0;
          G = 0;
          B = 0;
          cmdLength = 2;
          break;
          
        case 0x01: // All to RGB
          cmd[2] = GetNextByte(); // R
          cmd[3] = GetNextByte(); // G
          cmd[4] = GetNextByte(); // B
          R = cmd[2];
          G = cmd[3];
          B = cmd[4];
          cmdLength = 5;
          break;
          
        case 0x02: // All to HSI
          cmd[2] = GetNextByte(); // H Upper 
          cmd[3] = GetNextByte(); // H Lower
          cmd[4] = GetNextByte(); // S
          cmd[5] = GetNextByte(); // I
          // Convert HSI to R, G, B
          cmdLength = 6;
          break;
          
        case 0x03: // Read IR
          cmd[2] = GetNextByte(); // Address
          cmd[3] = GetNextByte(); // IR
          if (cmd[2] == myAddr) cmd[3] = IR_val;
          cmdLength = 4;
          break;
          
        case 0x04: // Max IR
          cmd[2] = GetNextByte(); // IR
          cmd[3] = GetNextByte(); // Address
          if (IR_val > cmd[2]) { 
            cmd[2] = IR_val;
            cmd[3] = myAddr;
          }
          cmdLength = 4;
          break;
          
        case 0x05: // Update nodes
          cmdLength = 2;
          break;
          
        case 0x06: // Enable IR
          cmd[2] = GetNextByte(); // Address
          if (cmd[2] == myAddr) IR_enabled = true;
          cmdLength = 3;
          break;
          
        case 0x07: // Disable IR
          cmd[2] = GetNextByte(); // Address
          if (cmd[2] == myAddr) IR_enabled = false;
          cmdLength = 3;
          break;
          
        case 0x08: // Enable all IR
          IR_enabled = true;
          cmdLength = 2;
          break;
          
        case 0x09: // Disable all IR
          IR_enabled = false;
          cmdLength = 2;
          break;
      }
      forward = true;
    } else {
      // cmd[0] not for this node or broadcast
      // Address to other nodes
      // Read R G B and store
      cmd[1] = GetNextByte(); // R
      cmd[2] = GetNextByte(); // G
      cmd[3] = GetNextByte(); // B
      cmdLength = 4;
      forward = true;
    }

    // Forward the command if required
    if (forward) {
      for (i = 0; i < cmdLength; i++) {
        mySerial.write(cmd[i]);
      }
      forward = false;
    }
    
  }

  setRGB(R, G, B);

  

  /*
  while (mySerial.available()) {
    buf[i] = mySerial.read();
    
    // If newline or brightness encountered or buffer full
    if (buf[i] == '\r' || buf[i] == '\n' || i==19) {
      i++;
      buf[i] = '\0';      // terminate the string
      serialReady = true; // mark for processing
      break;
    }
    
    i++;
  }
  */

  /*
  if (mySerial.available()) {
    while (true) {
      buf[i] = mySerial.read();
      if (buf[i] == '\r' || buf[i] == '\n') {
        i++;
        break;
      }
      i++;
    }
  }
  */

  /*
  // Consume the buffer if ready
  if (serialReady) {
    len = i; // length, also index of first empty character
  
    // Loop through all characters saved
    for (i = 0; i < len; i++) {
      // Check for CR / LF
      switch(buf[i]) {
        case 13: // CR
        case 10: // LF
        case 0:
          break;
        case 'H':
        case 'S':
        case 'I':
        case 'h':
        case 's':
        case 'i':
          // Detect mode and reset temp variable
          // Write out the last mode if it exists
          if (mode=='H' || mode=='h') H = temp;
          if (mode=='S' || mode=='s') S = temp;
          if (mode=='I' || mode=='i') I = temp;
          // Save the new mode
          mode = buf[i];
          temp = 0;
          break;
        case 'd':
          // IR
          mySerial.print("IR: ");
          mySerial.println(IR_val);
          break;
        default:
          // Detect numbers
          if (buf[i]>='0' && buf[i]<='9') {
            temp = temp*10; // move by 1 position
            temp = temp + (buf[i]-'0'); // Add the equivalent value
          }
      }
    }
            
    // Write out the last detected mode if it exists
    if (mode=='H' || mode=='h') H = temp;
    if (mode=='S' || mode=='s') S = temp;
    if (mode=='I' || mode=='i') I = temp;
  
    // Report out
    if (len > 0) {
      //mySerial.println(buf);
      mySerial.print("H ");
      mySerial.print(H);
      mySerial.print(" S ");
      mySerial.print(S);
      mySerial.print(" I ");
      mySerial.println(I);
    }

    // empty buffer
    for (i = 0; i <= len; i++) {
      buf[i] = '\0';
    }

    // reset
    i = 0;
    len = 0;
    mode = '\0';
    temp = 0;
  
    serialReady = false;
  }
  */
  

  // setHSI(H, 0.01 * S, 0.01 * I);
  
}


byte GetNextByte() {
  while (!mySerial.available()) {  }
  return mySerial.read();
}



float mod(float num, float div) {
  return num - (div * (int)(num / div));
}


void setRGB(int red_val, int green_val, int blue_val) {
  if (common_anode) {
    analogWrite(pinRed, 255-red_val);
    analogWrite(pinGreen, 255-green_val);
    analogWrite(pinBlue, 255-blue_val);
  } else {
    analogWrite(pinRed, red_val);
    analogWrite(pinGreen, green_val);
    analogWrite(pinBlue, blue_val);
  }
}
void setRGBpct(float red_val, float green_val, float blue_val) { 
  setRGB((red_val * 255), (green_val * 255), (blue_val * 255));
}

void setHSL(int hue, float sat, float lum) {
  if (sat == 0) {
    setRGBpct(lum, lum, lum);
    return;
  }

  float C = (1.0 - abs(2.0*lum - 1)) * sat;
  // printf("C = %5f ", C);

  float X = C * (1.0 - abs( mod( ((float)hue/60), 2) - 1.0) );
  // printf("X = %5f ", X);

  float m = lum - C/2.0;
  // printf("m = %5f \n", m);

       if (hue <  60) setRGBpct(C + m, X + m, 0 + m);
  else if (hue < 120) setRGBpct(X + m, C + m, 0 + m);
  else if (hue < 180) setRGBpct(0 + m, C + m, X + m);
  else if (hue < 240) setRGBpct(0 + m, X + m, C + m);
  else if (hue < 300) setRGBpct(X + m, 0 + m, C + m);
  else if (hue < 660) setRGBpct(C + m, 0 + m, X + m);
}



void setHSI(int hue, float saturation, float intensity) {
  int rgb[3];

  hsi2rgb(hue, saturation, intensity, rgb);

  setRGB(rgb[0], rgb[1], rgb[2]);
}

// From: http://blog.saikoled.com/post/43693602826/why-every-led-light-should-be-using-hsi
//
// Function example takes H, S, I, and a pointer to the 
// returned RGB colorspace converted vector. It should
// be initialized with:
//
// int rgb[3];
//
// in the calling function. After calling hsi2rgb
// the vector rgb will contain red, green, and blue
// calculated values.


void hsi2rgb(float H, float S, float I, int* rgb) {
  int r, g, b;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
    
  // Math! Thanks in part to Kyle Miller.
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }
  rgb[0]=r;
  rgb[1]=g;
  rgb[2]=b;
}

