#include <WiiChuck.h>

#include <BLEMidi.h>
Accessory guitar;

#define OCTAVE_OFFSET 12
#define OCTAVE_MAX 3
#define OCTAVE_MIN -3

#define CHANNEL_MAX 16

int pentaScale[] = {
  60,
  62,
  64,
  67,
  69
};

bool buttonState[5];
bool noteState[5];
bool strumState;
int prevWhammy, prevPlusMin, prevChannelBut;

int channel = 0;

int octave[CHANNEL_MAX];

void setup() {
  Serial.begin(115200);
  guitar.begin();
  if (guitar.type == Unknown) {
    /** If the device isn't auto-detected, set the type explicatly
		 * 	NUNCHUCK,
		 WIICLASSIC,
		 GuitarHeroController,
		 GuitarHeroWorldTourDrums,
		 DrumController,
		 DrawsomeTablet,
		 Turntable
		 */
    guitar.type = GuitarHeroController;
  }
  Serial.println("Initializing bluetooth");
  BLEMidiServer.begin("MIDI Hero");
  Serial.println("Waiting for connections...");
  BLEMidiServer.enableDebugging();  // Uncomment if you want to see some debugging output from the library (not much for the server class...)
}

void loop() {
  guitar.readData();  // Read inputs and update maps

  // Serial.println("-------------------------------------------");
  // guitar.printInputs(); // Print all inputs
  // for (int i = 0; i < WII_VALUES_ARRAY_SIZE; i++) {
  // 	Serial.println(
  // 			"Controller Val " + String(i) + " = "
  // 					+ String((uint8_t) guitar.values[i]));
  // }


  // Octave
  int plusMin = guitar.values[6];
  if (plusMin != prevPlusMin) {
    if (plusMin != 128) {
      octave[channel] += (plusMin < 128) ? -1 : 1;
      octave[channel] = max(min(octave[channel], OCTAVE_MAX), OCTAVE_MIN);
    }
    prevPlusMin = plusMin;
  }
  Serial.print("Octave:\t");Serial.println(octave[channel]);

  //Channel
  int joyY = guitar.getStickYGuitar();
  int channelBut = (joyY < 20)?-1:((joyY > 50)?1:0);
  if (channelBut != prevChannelBut) {
    BLEMidiServer.controlChange(channel, 123, 0);
    channel += channelBut;
    channel = max(min(channel, CHANNEL_MAX), 0);
    prevChannelBut = channelBut;
  }
  Serial.println(joyY);
  Serial.print("Channel:\t");Serial.println(channel);

  // Colour buttons
  for (int i = 0; i < 5; i++) {
    bool val = guitar.values[10 + i] == 255;
    if (val != buttonState[i]) {
      if (!val && noteState[i]) {
        BLEMidiServer.noteOff(channel, pentaScale[i] + octave[channel] * OCTAVE_OFFSET, 127);
        noteState[i] = false;
      }
      buttonState[i] = val;
    }
  }

  // Strum bar
  bool strum = guitar.values[7] != 128;
  if (strum != strumState) {
    if (strum) {
      for (int i = 0; i < 5; i++) {
        if (buttonState[i]) {
          if (noteState[i]) {
            BLEMidiServer.noteOff(channel, pentaScale[i] + octave[channel] * OCTAVE_OFFSET, 127);
          }
          BLEMidiServer.noteOn(channel, pentaScale[i] + octave[channel] * OCTAVE_OFFSET, 127);
          noteState[i] = true;
        }
      }
    }
    strumState = strum;
  }

  // Whammy bar
  int whammy = map(guitar.values[0], 16, 25, 8192, 12288);
  if (whammy != prevWhammy) {
    BLEMidiServer.pitchBend(channel, (uint16_t)whammy);
    prevWhammy = whammy;
  }
}
