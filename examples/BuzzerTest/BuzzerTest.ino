#include <Arduino.h>
#include "PassiveMelodyBuzzer.h"

#define BUZZER_PIN 21                     // IO Pin the buzzer is connected to
PassiveMelodyBuzzer buzzer(BUZZER_PIN);   // Create buzzer object


void setup() {
  Serial.begin(115200);                   
  for(int i = 0; i < 3; i++)              // For 3 times 
  {
    buzzer.click();                           // generate "click" sound
    while (buzzer.busy()) ;                   // wait for the click sound to finish
    delay(200);                               // wait 300 msec
  }
  buzzer.playMelody("C D E F G A B c");
}

void loop() {
  buzzer.loop();                          // Keep on playing
  String readLine;
  if (serialReadLine(readLine))           // A line from Serial available?
  {
    if (buzzer.busy())                       // if the buzzer is still playing the "old" melody, stop playback
      buzzer.stop();                         // Note that this is for demonstration only, starting a new melody with the line
                                             // below would cancel playback anyways...
    buzzer.playMelody(readLine.c_str(), 1);  // Feed it to the buzzer, to treat it as Melody
                                             // Second parameter will force some debug output on Serial
  }
}


bool serialReadLine(String& line)
{
  static String result = "";
  static char lastChar = 0;
  while (Serial.available())
  {
    char x = Serial.read();
    if (x >= ' ')
    {
      result += x;
      lastChar = x;
    }
    else if (((x == '\n') && (lastChar != '\r')) ||
             ((x == '\n') && (lastChar != '\r')))
    {
      line = result;
      line.trim();
      lastChar = x;
      return true;
    }
  }
  return false;
}
