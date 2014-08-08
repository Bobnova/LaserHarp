// This code is being released on the Creative Commons Attributation ShareAlike
// 4.0 International license.
// For information about this license, please visit the Creative Commons website
// at http://creativecommons.org/licenses/by-sa/4.0/




// Please note, this program is a BETA at best, it is not complete and has not been cleaned!
// If you use it in a project, expect to have to fix things.
// No warranty is expressed or implied, use at your own risk, etc.



// Number of physical strings. Connect strings to squentially to analog inputs (A0, A1, A2, etc.)
#define numberOfStrings  12
byte laserPin = PC_6;

boolean hardNoteCut = true;

byte MIDIVelocity = 69; // Strength of the string pluck. Adjustable somehow maybe?
byte noteOn = 0b10010000;
byte noteOff = 0b10000000;

byte stringNotes[numberOfStrings] {
  60,61,62,63,64,65,66,67,68,69,70,71};

byte stringPin[numberOfStrings] {
  A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11};

boolean stringStatus[numberOfStrings];
boolean lastStringStatus[numberOfStrings];

unsigned long stringThreshold[numberOfStrings];

byte MIDIAddresses[16];

byte currentString;

unsigned long noteThreshold[1];

int currentReading;

unsigned long programmingStartTime;
unsigned long programmingTimeout = 30000;


byte activeStringCount = 0;
byte LEDLevels[3][13] = {
  {
    0,0,    0,255, 127,    0,127,127,64 ,0  ,64  ,64  ,255}
  ,
  {
    0,255,  0,  0, 127,  127, 0,64,192,255,192,64,255}
  ,
  {
    0, 0,  255,   0, 0,   127,  127  ,  127,  0,  0, 64,127,255}
};




void setup()
{
  // Can't declare serial here, only one baud rate at a time is supported.
  //Serial.begin(115200);
  // Serial2.begin(31250); // MIDI Baud rate
  for (byte x = 0; x < 16; x++){
    MIDIAddresses[x] = 255;
  }

  pinMode(laserPin,OUTPUT);
  digitalWrite(laserPin,HIGH);


  //This bit is not Arduino compatible, at all. Arduino EEPROM is different.
  ROM_EEPROMRead(stringThreshold,50,sizeof(stringThreshold));
  unsigned long tempVelocity[1];
  ROM_EEPROMRead(tempVelocity,10,sizeof(tempVelocity));
  MIDIVelocity = tempVelocity[0];
  pinMode(PUSH1,INPUT_PULLUP);
  pinMode(PUSH2,INPUT_PULLUP);


}

void loop(){

  if(!digitalRead(PUSH1)){
    programmingMode();
  }
  if(!digitalRead(PUSH2)){
    setThresholds();
  }
  lastStringStatus[currentString] = stringStatus[currentString];
  currentReading = 0;
  for (byte x = 0; x < 5; x++){
    currentReading += analogRead(stringPin[currentString]);
    delayMicroseconds(5);
  }
  currentReading /=5;
  if (currentReading > stringThreshold[currentString]){
    stringStatus[currentString] = 1;
  }
  else{
    stringStatus[currentString] = 0;
  }
  if (stringStatus[currentString] && !lastStringStatus[currentString] && hardNoteCut){
    for (byte x = 0; x < 16; x++){
      if (MIDIAddresses[x] == 255){
        MIDIAddresses[x] = currentString;
        playNote(noteOn,MIDIAddresses[x],stringNotes[currentString],MIDIVelocity);
        activeStringCount++;
        x = 200;
      }
    }
  }
  else if (!stringStatus[currentString] && lastStringStatus[currentString] && hardNoteCut){
    for (byte x = 0; x < 16; x++){
      if (MIDIAddresses[x] == currentString){
        playNote(noteOff,MIDIAddresses[x],stringNotes[currentString],MIDIVelocity);
        MIDIAddresses[x] = 255;
        activeStringCount--;
      }
    }
  }
  currentString++;
  if (currentString > numberOfStrings - 1){
    currentString = 0;
  }
  setLEDLevels();
}



void playNote(byte onOff, byte address, byte note, byte velocity){

  // The Serial and Serial2 begin and end statements are required due to having diferent baud rates.
  // On the final design only Serial2 (MIDI) will be used, Serial2.begin will be called in setup()
  // and simply left alone after that. Serial2.flush() will be removed as well, in the interest
  // of loop speed. The buffer is large enough to hold a tremendous number of notes before it overflows.

  // Put the note on / note off together with the address to make the control byte.
  byte control = onOff | address; 

  Serial2.begin(31250);  
  Serial2.write(control);
  Serial2.write(note);
  Serial2.write(velocity);
  Serial2.flush();
  Serial2.end();

  Serial.begin(115200);
  Serial.print(control);
  Serial.print(",");
  Serial.print(note);
  Serial.print(",");
  Serial.print(velocity);
  Serial.println(".");
  Serial.flush();
  Serial.end();

}

// The threshold part of programming mode is no longer useful. It will be removed
// in the future. The MIDI Velocity part is still useful, hence the section existing.
void programmingMode(){
  digitalWrite(laserPin,LOW);
  byte firstComma;
  byte secondComma;
  unsigned long tempThreshold[1] = {
    0      };
  unsigned long tempVelocity[1] = {
    0      };
  programmingStartTime = millis();
  String programmingString = "";
  Serial.begin(115200);
  Serial.println(noteThreshold[0]);
  Serial.println(MIDIVelocity);
  Serial.println("Programming mode.");
  Serial.println("Format: HLH,threshold,velocity,HLH");
  Serial.println("Threshold is 0-4096, velocity is 0-127. HLH must be capitalized.");
  Serial.println("Programming mode will time out in 30 seconds.");
  while(millis() - programmingStartTime < programmingTimeout){
    if(Serial.available()){
      delay(100);
      while(Serial.available() > 0){

        char inChar = char(Serial.read());
        programmingString += inChar;
      }

      if(programmingString.startsWith("HLH") && programmingString.endsWith("HLH") && programmingString.length() > 10){
        firstComma = programmingString.lastIndexOf(',');
        secondComma = programmingString.lastIndexOf(',',firstComma-1);

        if (programmingString.charAt(secondComma-1) - 48 > -1 && programmingString.charAt(secondComma-1) - 48 < 11){
          tempThreshold[0] = programmingString.charAt(secondComma-1) - 48;
        }
        if (programmingString.charAt(secondComma-2) - 48 > -1 && programmingString.charAt(secondComma-2) - 48 < 11){
          tempThreshold[0] += (programmingString.charAt(secondComma-2) - 48) * 10;
        }
        if (programmingString.charAt(secondComma-3) - 48 > -1 && programmingString.charAt(secondComma-3) - 48 < 11){
          tempThreshold[0] += (programmingString.charAt(secondComma-3) - 48) * 100;
        }
        if (programmingString.charAt(secondComma-4) - 48 > -1 && programmingString.charAt(secondComma-4) - 48 < 11){
          tempThreshold[0] += (programmingString.charAt(secondComma-4) - 48) * 1000;
        }


        ROM_EEPROMProgram(tempThreshold,0,sizeof(tempThreshold[0]));
        noteThreshold[0] = tempThreshold[0];
        Serial.println(noteThreshold[0]);

        if (programmingString.charAt(firstComma-1) - 48 > -1 && programmingString.charAt(firstComma-1) - 48 < 11){
          tempVelocity[0] = programmingString.charAt(firstComma-1) - 48;
        }
        if (programmingString.charAt(firstComma-2) - 48 > -1 && programmingString.charAt(firstComma-2) - 48 < 11){
          tempVelocity[0] += (programmingString.charAt(firstComma-2) - 48) * 10;
        }
        if (programmingString.charAt(firstComma-3) - 48 > -1 && programmingString.charAt(firstComma-3) - 48 < 11){
          tempVelocity[0] += (programmingString.charAt(firstComma-3) - 48) * 100;
        }
        if (tempVelocity[0] > 127){
          tempVelocity[0] = 127;
        }
        ROM_EEPROMProgram(tempVelocity,10,sizeof(tempVelocity[0]));
        MIDIVelocity = tempVelocity[0];
        Serial.println(MIDIVelocity);
        Serial.end();
        digitalWrite(laserPin,HIGH);
        break;
      }
    }

  }
  Serial.end();
  digitalWrite(laserPin,HIGH);
}

void setLEDLevels(){
  analogWrite(RED_LED,LEDLevels[0][activeStringCount]);
  analogWrite(GREEN_LED,LEDLevels[1][activeStringCount]);
  analogWrite(BLUE_LED,LEDLevels[2][activeStringCount]);
}

void setThresholds(){
  unsigned long laserOffReadings[numberOfStrings];
  unsigned long laserOnReadings[numberOfStrings];
  digitalWrite(laserPin,LOW);
  delay(10);
  for (byte y = 0; y < numberOfStrings; y++){
    unsigned long tempReading = 0;
    for (byte x = 0; x < 5; x++){
      tempReading += analogRead(stringPin[y]);
      delayMicroseconds(5);
    }
    tempReading /= 5;
    laserOffReadings[y] = tempReading;
  }
  digitalWrite(laserPin,HIGH);
  delay(10);
  for (byte y = 0; y < numberOfStrings; y++){
    unsigned long tempReading = 0;
    for (byte x = 0; x < 5; x++){
      tempReading += analogRead(stringPin[y]);
      delayMicroseconds(5);
    }
    tempReading /= 5;
    laserOnReadings[y] = tempReading;
  }
  for (byte y = 0; y < numberOfStrings; y++){
    stringThreshold[y] = (laserOnReadings[y] + laserOffReadings[y]) / 2;
    Serial.print(laserOnReadings[y]);
    Serial.print(",");
    Serial.print(laserOffReadings[y]);
    Serial.print(",");
    Serial.print(stringThreshold[y]);
    Serial.println("");
  }
  ROM_EEPROMProgram(stringThreshold,50,sizeof(stringThreshold));
  while(!digitalRead(PUSH2)){
    while(!digitalRead(PUSH2)){
      delay(500);
    }
    delay(500);
  }

}


