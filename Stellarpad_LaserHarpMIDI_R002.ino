
byte numberOfStrings = 6; // Number of physical strings. Connect strings to squentially to analog inputs (A0, A1, A2, etc.)

boolean hardNoteCut = true;

byte MIDIVelocity = 0x45; // Strength of the string pluck. Adjustable somehow maybe?
byte noteOn = 0b10010000;
byte noteOff = 0b10000000;

byte stringNotes[12] {60,61,62,63,64,65,66,67,68,69,70,71};

byte stringPin[12] {A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11};

boolean stringStatus[12];
boolean lastStringStatus[12];

byte MIDIAddresses[16];

byte currentString;

int noteThreshold = 2048;
int currentReading;





void setup()
{
  //Serial.begin(115200);
 // Serial2.begin(31250); // MIDI Baud rate
 for (byte x = 0; x < 16; x++){
   MIDIAddresses[x] = 255;
 }
   
}

void loop(){
  
  lastStringStatus[currentString] = stringStatus[currentString];
  currentReading = 0;
  for (byte x = 0; x < 5; x++){
    currentReading += analogRead(stringPin[currentString]);
    delayMicroseconds(5);
  }
  currentReading /=5;
  if (currentReading < noteThreshold){
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
        x = 200;
      }
    }
  }
  else if (!stringStatus[currentString] && lastStringStatus[currentString] && hardNoteCut){
    for (byte x = 0; x < 16; x++){
      if (MIDIAddresses[x] == currentString){
        playNote(noteOff,MIDIAddresses[x],stringNotes[currentString],MIDIVelocity);
        MIDIAddresses[x] = 255;
      }
    }
  }
  
  currentString++;
  if (currentString > numberOfStrings - 1){
    currentString = 0;
  }
  
      
}



void playNote(byte onOff, byte address, byte note, byte velocity){
  
  // The Serial and Serial2 begin and end statements are required due to having diferent baud rates.
  // On the final design only Serial2 (MIDI) will be used, Serial2.begin will be called in setup()
  // and simply left alone after that. Serial2.flush() will be removed as well, in the interest
  // of loop speed. The buffer is large enough to hold a tremendous number of notes before it overflows.
  
  byte control = onOff | address; // Put the note on / note off together with the address to make the control byte.
  
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


