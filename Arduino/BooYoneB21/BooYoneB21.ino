
#include <Arduino_Helpers.h>
#include <AH/Hardware/FilteredAnalog.hpp>

#include <MIDIUSB_Defs.h>
#include <MIDIUSB.h>
#include <pitchToNote.h>
#include <frequencyToNote.h>
#include <pitchToFrequency.h>

//            ----------------------------------------------
// -----------=============   C O N S T A N T S   ==========-----------------
//            ----------------------------------------------

// MIDI Channel and default Velocity to be used
#define CHANNEL    0
#define VELOCITY 127

// ************************   B U T T O N S   *************************

// ######## ------- KeyBoard Matrix -------- ################################
#define COLS 4
#define ROWS 4

// inPins - in columns - read the state of input
const int inPin[COLS]  = { 4,  5,  6,  7};
// outPins- in rows -  set HIGH/LOW on the output
const int outPin[ROWS] = { 8, 10, 16, 14};

// MIDI codes of buttons in a matrix
const byte buttonMidiKeys[ROWS][COLS] = {
      {0x30, 0x31, 0x32, 0x33},
      {0x34, 0x35, 0x36, 0x37},
      {0x38, 0x39, 0x3A, 0x3B},
      {0x3C, 0x3D, 0x3E, 0x3F},
  };

// ************************   P O T E N T I O M E T E R S   *************************

// border positions of tempo driving potentiometrs
// 0 - 99 - 311 -  711 - 1023
const int brdrSSlow = 100;
const int brdrSlow  = 312;
const int brdrFast  = 712;
const int brdrSFast = 925;

// Tempo potentiometr value mapped to following button/key values
// corresponds to change of speed in percents
const int btnSSlow = -4;
const int btnSlow  = -2;
const int btnNone  =  0;
const int btnFast  =  2;
const int btnSFast =  4;

// border positions of LoopSize driving potentiometrs
// 0 - 255 - 511 - 767 - 1023
const int brdrL02 = 256;
const int brdrL04 = 512;
const int brdrL08 = 768;

// LoopSize potentiometr values mapped to following button/key values
const int btnL02 = 0;
const int btnL04 = 1;
const int btnL08 = 2;
const int btnL16 = 3;
const int btnL32 = 4;

// ######## ------- Analog Inputs - pins -------- ###########################

const int leftLoopAnalogPin   = A0;
const int rightLoopAnalogPin  = A2;
const int leftTempoAnalogPin  = A1;
const int rightTempoAnalogPin = A3;

// MIDI codes for values of potentiometers
const int leftLoopBaseMidiKey  = 0x08;  // -> 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D (reserved)
const int leftTempoBaseMidiKey = 0x24;  // -> 0x20, 0x22, 0x24, 0x26, 0x28

const int rightLoopBaseMidiKey  = 0x0E; // -> 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 (reserved)
const int rightTempoBaseMidiKey = 0x25; // -> 0x21, 0x23, 0x25, 0x27, 0x29

//            ----------------------------------------------
// -----------=============   V A R I A B L E S   ==========-----------------
//            ----------------------------------------------


// ************************   B U T T O N S   *************************

// (previous) read states of buttons in a matrix
byte buttonState[ROWS][COLS] = {
      {HIGH, HIGH, HIGH, HIGH},
      {HIGH, HIGH, HIGH, HIGH},
      {HIGH, HIGH, HIGH, HIGH},
      {HIGH, HIGH, HIGH, HIGH},
  };

// read states of buttons in a matrix
byte buttonRead[ROWS][COLS] = {
      {HIGH, HIGH, HIGH, HIGH},
      {HIGH, HIGH, HIGH, HIGH},
      {HIGH, HIGH, HIGH, HIGH},
      {HIGH, HIGH, HIGH, HIGH},
  };

// ************************   P O T E N T I O M E T E R S   *************************

FilteredAnalog<> leftLoopAnalogPot   = leftLoopAnalogPin;
FilteredAnalog<> rightLoopAnalogPot  = rightLoopAnalogPin;
FilteredAnalog<> leftTempoAnalogPot  = leftTempoAnalogPin;
FilteredAnalog<> rightTempoAnalogPot = rightTempoAnalogPin;

int leftTempoValue     = leftTempoBaseMidiKey;
int rightTempoValue    = rightTempoBaseMidiKey;
int leftLoopSizeValue  = leftLoopBaseMidiKey;
int rightLoopSizeValue = rightLoopBaseMidiKey;

//            ----------------------------------------------
// -----------===============     S E T U P     ============-----------------
//            ----------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(10);
  while (!Serial);

  // Select the correct ADC resolution
  FilteredAnalog<>::setupADC();
  initPinModes();
  delay(1);
  initLoopPotentiometers();
  delay(1);
  initTempoPotentiometers();
}


//            ----------------------------------------------
// -----------===============     L O O O P     ============-----------------
//            ----------------------------------------------

void loop() {
  handleButtons();
  handlePotentiometers();
  MidiUSB.flush();
}

// ---- loop functions ---------------------------------------------------------

void handleButtons() {
  readKeypadRow(0);
  readKeypadRow(1);
  readKeypadRow(2);
  readKeypadRow(3);
}

// ----  LOW -> button is pressed  ----
void readKeypadRow(int row) {
    int buttRead = 0;
    digitalWrite(outPin[row], LOW);

    for (byte col = 0 ; col < COLS ; col++) {
        buttRead = digitalRead(inPin[col]);
        // if (LOW == buttRead) { printLow(row,col); }
        if (    (buttRead == buttonRead[row][col])
             && (buttRead != buttonState[row][col]) ) {
            switch (buttRead) {
                case LOW:             // -> button is currently pressed
                    noteOn(buttonMidiKeys[row][col]);
                    // printPressed(row, col);
                    buttonState[row][col] = LOW;
                    break;
                default: // == HIGH   // -> button is currently released
                    noteOff(buttonMidiKeys[row][col]);
                    // printRlsd(row, col);
                    buttonState[row][col] = HIGH;
                    break;
            }
        }
        buttonRead[row][col] = buttRead;
    }
    digitalWrite(outPin[row], HIGH);
}


//              ----------------------------------------------
// -----------=========== P O T E N T I O M E T E R S =========-----------------
//              ----------------------------------------------

void handlePotentiometers() {
  handleTempoAnalogPots();
  handleLoopAnalogPots();
}

// Tempo Analog Potentiometer
void handleTempoAnalogPots() {
    // LEFT Tempo Pot
    if (leftTempoAnalogPot.update()) {
        int leftAnalogValue = leftTempoAnalogPot.getValue();
        int mappedValue = mapToTempoButton(leftAnalogValue);
        int mappedButton = leftTempoBaseMidiKey + mappedValue;

        // check if mapped value changed from last tempo value
        if (    (leftTempoBaseMidiKey == mappedButton)
             && (leftTempoBaseMidiKey != leftTempoValue)) {
            // 0 -> turn off all tempos, just 2B sure
            // Serial.print("LTempoPot #");
            // Serial.print(leftTempoAnalogPin);
            // Serial.print(" V ");
            // Serial.print(leftAnalogValue);
            // Serial.print(": ");
            // Serial.print(leftTempoValue);
            // Serial.print(" -> ");
            // Serial.print(mappedButton);
            // Serial.println(" ~ 0 -> noteOff(ALL)");
            noteOff(0x20);
            noteOff(0x22);
            noteOff(0x26);
            noteOff(0x28);
            leftTempoValue = mappedButton;
            // TODO LEDs : setTempoLedColor(leftTempoLed, mappedValue);
        } else if (mappedButton != leftTempoValue) {
            // new tempo value ->
            // Serial.print("LTempoPot #");
            // Serial.print(leftTempoAnalogPin);
            // Serial.print(" V ");
            // Serial.print(leftAnalogValue);
            // Serial.print(": ");
            // Serial.print(leftTempoValue);
            // Serial.print(" -> noteOn ");
            // Serial.print(mappedButton, HEX);
            // 1. turn off previous key
            //    !!! do not turn off ZERO ~ leftTempoBaseMidiKey !!!
            if (leftTempoValue != leftTempoBaseMidiKey) {
                // Serial.print(" : In need to noteOff(");
                // Serial.print(leftTempoValue, HEX);
                noteOff(leftTempoValue);
                // Serial.println(")");
            } else {
                // No need to noteOff(baseMidiKey)
                // Serial.println();
            }
            // 2. turn on new key
            noteOn(mappedButton);
            leftTempoValue = mappedButton;
            // TODO LEDs : setTempoLedColor(leftTempoLed, mappedValue);
        }
        // else nothing -> mappedButton is same as last tempo value
    }
    // RIGHT Tempo Pot
    if (rightTempoAnalogPot.update()) {
        int rightAnalogValue = rightTempoAnalogPot.getValue();
        int mappedValue = mapToTempoButton(rightAnalogValue);
        int mappedButton = rightTempoBaseMidiKey + mappedValue;

        // int mappedButton = mapToTempoButton(rightAnalogValue);
        // check if mapped value changed from last tempo value
        if ((rightTempoBaseMidiKey == mappedButton) && (rightTempoBaseMidiKey != rightTempoValue)) {
            // 0 -> turn off all tempos, just 2B sure
            // Serial.print("RTempoPot #");
            // Serial.print(rightTempoAnalogPin);
            // Serial.print(" V ");
            // Serial.print(rightAnalogValue);
            // Serial.println(": ~ 0 -> noteOff(ALL)");
            noteOff(0x21);
            noteOff(0x23);
            noteOff(0x27);
            noteOff(0x29);
            rightTempoValue = mappedButton;
            // TODO LEDs : setTempoLedColor(rightTempoLed, mappedValue);
        } else if (mappedButton != rightTempoValue) {
            // new tempo value ->
            // Serial.print("RTempoPot #");
            // Serial.print(rightTempoAnalogPin);
            // Serial.print(" V ");
            // Serial.print(rightAnalogValue);
            // Serial.print(": ");
            // Serial.print(rightTempoValue);
            // Serial.print(" -> noteOn ");
            // Serial.print(mappedButton, HEX);
            // 1. turn off previous key
            //    !!! do not turn off ZERO ~ rightTempoBaseMidiKey !!!
            if (rightTempoValue != rightTempoBaseMidiKey) {
                // Serial.print(" : In need to noteOff(");
                // Serial.print(rightTempoValue, HEX);
                noteOff(rightTempoValue);
                // Serial.println(")");
            } else {
              // No need to noteOff(baseMidiKey)
                // Serial.println();
            }
            // 2. turn on new key
            noteOn(mappedButton);
            rightTempoValue = mappedButton;
            // TODO LEDs : setTempoLedColor(rightTempoLed, mappedValue);
        }
        // else nothing -> mappedButton is same as last tempo value
    }
}

// LoopSize Analog Potentiometers
void handleLoopAnalogPots() {
    // LEFT LoopSize Pot
    if (leftLoopAnalogPot.update()) {
        int leftAnalogValue = leftLoopAnalogPot.getValue();
        int mappedValue = mapToLoopSizeButton(leftAnalogValue);
        int mappedButton = leftLoopBaseMidiKey + mappedValue;

        if (mappedButton != leftLoopSizeValue) {
            // new tempo value ->
            // Serial.print("LLoopSize Pot #");
            // Serial.print(leftLoopAnalogPin);
            // Serial.print(" V ");
            // Serial.print(leftAnalogValue);
            // Serial.print(": noteOff ");
            // Serial.print(leftLoopSizeValue);
            // Serial.print(" -> noteOn ");
            // Serial.println(mappedButton, HEX);
            // 1. turn off previous key
            noteOff(leftLoopSizeValue);
            // 2. turn on new key
            noteOn(mappedButton);
            leftLoopSizeValue = mappedButton;
            // TODO LEDs : setLoopSizeLedColor(leftLoopLed, mappedValue);
        }
        // else nothing -> mappedButton is same as last tempo value
    }
    // RIGHT LoopSize Pot
    if (rightLoopAnalogPot.update()) {
        int rightAnalogValue = rightLoopAnalogPot.getValue();
        int mappedValue = mapToLoopSizeButton(rightAnalogValue);
        int mappedButton = rightLoopBaseMidiKey + mappedValue;

        if (mappedButton != rightLoopSizeValue) {
            // new tempo value ->
            // Serial.print("RLoopSize Pot #");
            // Serial.print(rightLoopAnalogPin);
            // Serial.print(" V ");
            // Serial.print(rightAnalogValue);
            // Serial.print(": noteOff ");
            // Serial.print(rightLoopSizeValue);
            // Serial.print(" -> noteOn ");
            // Serial.println(mappedButton, HEX);
            // 1. turn off previous key
            noteOff(rightLoopSizeValue);
            // 2. turn on new key
            noteOn(mappedButton);
            rightLoopSizeValue = mappedButton;
            // TODO LEDs : setLoopSizeLedColor(rightLoopLed, mappedValue);
        }
        // else nothing -> mappedButton is same as last tempo value
    }
}

// maps tempo potentiometr position to button value - corresponds to change of speed in percents
int mapToTempoButton(int analogValue) {
  if (brdrSSlow > analogValue) {
    return btnSSlow;
  }
  if (brdrSlow > analogValue) {
    return btnSlow;
  }
  if (brdrFast > analogValue) {
    return btnNone;
  }
  if (brdrSFast > analogValue) {
    return btnFast;
  }
  return btnSFast;
}

// maps tempo potentiometr position to button value - corresponds to change of a key from base 0 key
int mapToLoopSizeButton(int analogValue) {
  if (brdrL02 > analogValue) {
    return btnL02;
  }
  if (brdrL04 > analogValue) {
    return btnL04;
  }
  if (brdrL08 > analogValue) {
    return btnL08;
  }
  return btnL16;
}


//              ----------------------------------------------
// -----------===========  M I D I   U S B   C T R L  ==========---------------
//              ----------------------------------------------

// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

//  void noteOn(byte channel, byte pitch, byte velocity) {
//    midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
void noteOn(byte pitch) {
  midiEventPacket_t noteOn = {0x09, 0x90 | CHANNEL, pitch, VELOCITY};
  MidiUSB.sendMIDI(noteOn);
}

//  void noteOff(byte channel, byte pitch, byte velocity) {
//    midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
void noteOff(byte pitch) {
  midiEventPacket_t noteOff = {0x08, 0x80 | CHANNEL, pitch, VELOCITY};
  MidiUSB.sendMIDI(noteOff);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

//    //  void controlChange(byte channel, byte control, byte value) {
//    //  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
//    void controlChange(byte control, byte value) {
//      midiEventPacket_t event = {0x0B, 0xB0 | CHANNEL, control, value};
//      MidiUSB.sendMIDI(event);
//      // MidiUSB.flush()
//    }

//              -----------------------------------------------------------------
// -------------===============     S E T U P   F U N C T I O N S    ============-------------------
//              -----------------------------------------------------------------

void initPinModes() {
    // set inPins
    for (int i = 0; i < COLS; i++) {
        pinMode(inPin[i], INPUT_PULLUP);
    }
    // set outPins
    for (int i = 0; i < ROWS; i++) {
        pinMode(outPin[i], OUTPUT);
        digitalWrite(outPin[i], HIGH);
    }
}


//              -----------------------------------------------------------------

// Initialize the filter to whatever the value on the input is right now
// (otherwise, the filter is initialized to zero and you get transients)

void initLoopPotentiometers() {
    leftLoopAnalogPot.resetToCurrentValue();
    rightLoopAnalogPot.resetToCurrentValue();
    delay(1);

    leftLoopAnalogPot.update();
    int analogValue = leftLoopAnalogPot.getValue();
    int mappedValue = mapToLoopSizeButton(analogValue);
    leftLoopSizeValue = leftLoopBaseMidiKey + mappedValue;

    Serial.print("Current LeftLoopValue A/M ");
    Serial.print(analogValue);
    Serial.print("/");
    Serial.print(leftLoopSizeValue, HEX);
    Serial.print(" from PIN# ");
    Serial.println(leftLoopAnalogPin);

    rightLoopAnalogPot.update();
    analogValue = rightLoopAnalogPot.getValue();
    mappedValue = mapToLoopSizeButton(analogValue);
    rightLoopSizeValue = rightLoopBaseMidiKey + mappedValue;

    Serial.print("Current RightLoopValue A/M ");
    Serial.print(analogValue);
    Serial.print("/");
    Serial.print(rightLoopSizeValue, HEX);
    Serial.print(" from PIN# ");
    Serial.println(rightLoopAnalogPin);
}

void initTempoPotentiometers() {
    leftTempoAnalogPot.resetToCurrentValue();
    rightTempoAnalogPot.resetToCurrentValue();
    delay(1);

    leftTempoAnalogPot.update();
    int analogValue = leftTempoAnalogPot.getValue();
    int mappedValue = mapToTempoButton(analogValue);
    leftTempoValue = leftTempoBaseMidiKey + mappedValue;

    Serial.print("Current LeftTempoValue A/M ");
    Serial.print(analogValue);
    Serial.print("/");
    Serial.print(leftTempoValue, HEX);
    Serial.print(" from PIN# ");
    Serial.println(leftTempoAnalogPin);

    rightTempoAnalogPot.update();
    analogValue = rightTempoAnalogPot.getValue();
    mappedValue = mapToTempoButton(analogValue);
    rightTempoValue = rightTempoBaseMidiKey + mappedValue;

    Serial.print("Current RightTempoValue A/M ");
    Serial.print(analogValue);
    Serial.print("/");
    Serial.print(rightTempoValue, HEX);
    Serial.print(" from PIN# ");
    Serial.println(rightTempoAnalogPin);
}


// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------

// ---- DEBUG ------------------------------------------
void printLow(int row, int col) {
  Serial.print("[");
  Serial.print(row);
  Serial.print("][");
  Serial.print(col);
  Serial.print("] ");
  Serial.print(buttonMidiKeys[row][col]);
  Serial.println(" is LOW");
}

void printPressed(int row, int col) {
  Serial.print("[");
  Serial.print(row);
  Serial.print("][");
  Serial.print(col);
  Serial.print("] ");
  Serial.print(buttonMidiKeys[row][col]);
  Serial.println(" pressed");
}

void printRlsd(int row, int col) {
  Serial.print("[");
  Serial.print(row);
  Serial.print("][");
  Serial.print(col);
  Serial.print("] ");
  Serial.print(buttonMidiKeys[row][col]);
  Serial.println(" released");
}

void oldHandlePotentiometers() {
  int analogValue = 0;
  if (leftLoopAnalogPot.update()) {
    analogValue = leftLoopAnalogPot.getValue();
    Serial.print("leftLoopAnalogPot ");
    Serial.println(analogValue);
  }
  if (rightLoopAnalogPot.update()) {
    analogValue = rightLoopAnalogPot.getValue();
    Serial.print("rightLoopAnalogPot ");
    Serial.println(analogValue);
  }
  if (leftTempoAnalogPot.update()) {
    analogValue = leftTempoAnalogPot.getValue();
    Serial.print("leftTempoAnalogPot ");
    Serial.println(analogValue);
  }
  if (rightTempoAnalogPot.update()) {
    analogValue = rightTempoAnalogPot.getValue();
    Serial.print("rightTempoAnalogPot ");
    Serial.println(analogValue);
  }
}

// -- End of File --

