#include <Arduino.h>
#include <PassiveMelodyBuzzer.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <PubSubClient.h>

/*
 * Extends BuzzerSerial (try this one first) to allow sending melodies over MQTT.
 * For this you have to add your WiFi credentials. 
 * 
 * Also change the MQTT Broker settings if you want to use another MQTT broker.
 * Library PubSubClient from knolleary needs to be installed
 * 
 */

#if defined(ESP32)
#define BUZZER_PIN 21                     // IO Pin the buzzer is connected to 
                                          // (AZ-Touch mod has it connected to GPIO 21 for ESP32 DevKit)
#elif defined(ESP8266)
#define BUZZER_PIN  16                    // IO Pin the buzzer is connected to                                           
                                          // (AZ-Touch mod has it connected to GPIO 16 of Wemos D1 mini)
#endif

#define WIFI_SSID "YourWiFi SSID"   
#define WIFI_PASS "YourWiFi Password"


#define MQTT_BROKER (const char *)NULL    // Or set MQTT-Broker
#define MQTT_PORT   1883
#define MQTT_USER   (const char *)NULL    // Or set user name for MQTT broker access if needed
#define MQTT_PASS   (const char *)NULL    // Or set user password for MQTT broker access if needed

#define MQTT_TOPIC  "buzzer/playMelody"   // topic to subscribe to.

const char* pinkPanther = "-1!=130!/2#de*2.r#fg*2.r#de.#fg.c1b.eg.b#a*4!/3ragede*7.!/2Rr#de*2.r#fg*2.r#de.#fg.c1b.gb.e1#d1*9.RR"
                          "#de*2.r#fg*2.r#de.#fg.c1b.eg.b#a*4!/3ragede*7.!/2RRe1.d1b.ag.e.!/4#aa*3,#aa*3,#aa*3#aa*3!/3ged!/2e,e*5,";

const char* vamos = "!=600c1r!*2c1a+1c!*dr!*2drdrded!*3c";

PassiveMelodyBuzzer buzzer;


WiFiClient wifiClient;
PubSubClient client(wifiClient);


void setup() {
  buzzer.begin(BUZZER_PIN);
  Serial.begin(115200);                   
  Serial.println();Serial.println();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while ((WL_CONNECTED != WiFi.status()) && (millis() < 10000))
  { 
    if (!buzzer.busy())
      buzzer.playMelody(pinkPanther);
  }
  buzzer.stop();
  if (WL_CONNECTED != WiFi.status())
    Serial.println("WiFi Fail!");
  else if (NULL != MQTT_BROKER)
  {
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(callback);
  }
}


void loop() {
  buzzer.loop();                          // Keep on playing 
  if ((NULL != MQTT_BROKER) && (WL_CONNECTED == WiFi.status()))
    if (!client.connected())
      reconnect();
    else
      client.loop();
  String readLine;
  if (serialReadLine(readLine))           // A line from Serial available?
  {
    buzzer.playMelody(readLine, 1);         // Feed it to the buzzer, to treat it as Melody
                                             // Second parameter will force some debug output on Serial
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
char s[length + 1];
  memcpy(s, payload, length);
  s[length] = 0;
  buzzer.playMelody(s);

  Serial.printf("MQTT-Message arrived [%s] %s\r\n", topic, s);
}


bool reconnect() {
  bool ret;
  // Loop until we're reconnected
  ret = client.connected();
  if (!ret)
    {
    char s[50];
    sprintf(s, "buz%ld%ld", random(LONG_MAX), random(LONG_MAX));
    Serial.print("Attempting MQTT connection as ");Serial.print(s);Serial.print("...");
    // Attempt to connect
    if (ret = client.connect(s, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(MQTT_TOPIC);
      ret = true;
    } else {
      Serial.printf("failed, rc=%d\r\n", client.state());
      buzzer.playMelody(vamos);
      while (buzzer.busy())
        ;
      // Wait 5 seconds before retrying
      }
    }
  return ret;
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
      result = "";
      return true;
    }
  }
  return false;
}

/*
 * 
 You can paste the folling examples to the Serial monitor input.
 
 C-Major Scale with no spaces
 (All possible base tones, from lowest to highest, are CDEFGABcdefgab)
 (a is tuned to 440Hz, A is 220Hz)
 (spaces between notes have no effect but increase readeability, CEG will play the same as C    E G or CE G)
 CDEFGABc

 

 C-Major Scale with detached notes
 (This will not change the overall beat, just the tone is shortened a bit and replaced by silence)
 (Spaces are not permitted between the name of the note and the detachement suffix!)
 C, D, E, F, G, A, B, c

 C-Major Scale with even more detached notes
 (Same beat, just more silence)
 (Spaces are not permitted between the name of the note and the detachement suffix!)
 C. D. E. F. G. A. B. c
 
 C-Major Scale with alternating long and short breaks (rests) between notes (R has double length)
 C r D R E r F R F r A R B r c

 C-Major Scale with changed length of (some) notes
 (Spaces are not permitted between the name of the note and the length changing suffix!)
 (/2 will half the length, *2 will double)
 C/2 D/2 E/2 F G/3 A/3 B/3 c*2

 C-Major Scale with transpositions for a second octave 
 (Spaces are not permitted between the name of the note and the octave changing suffix!)
 (The maximum number for transposition is 4, any higher number like c5 will be ignored) 
 (Transposition can only be used for lowercase tone names. I. e. C1 C2 C3 has no effect and will result in C)
 c d e f g a b c1 d1 e1 f1 g1 a1 b1 c2

 C-Major Scale with transposition, detachment and changed length 
 The follwing order of suffix parts must be maintained:
  *If (optional) transposition is to be used, the octave number must be given first
  *If (optional) the length of a note shall be changed, / or * (followed by an optional number) or any combination of these must be given next
  *If (optional) detachment breaks shall be used, they must be given last
  *No space (or any other undefined character) is permitted in the suffix section at all
 c1/2, d1/2, e1/2, f1/2, g1/2, a1/2, b1/2, c2/2*5
 
 Length of rests can also be changed (but then the base length of 'R' will be the same as 'r' and not double)
 (Note that multplication and division can be combined in either order, so *4/2 is the same as /2*4 and effectively 2)
 (However that can be used to generate fractions like *2/5, not used in the example below)
 (Octave transpositions and detachement have no effect for breaks)
 (So R1*4/2. is the same as R*4/2. is the same as r*4/2. is the same as r*4/2, is the same as r*4/2. is the same as r*4/2 or r*2 or R)
 c1 d1 e1 f1 g1 r a1 R*4/2. b1 r1*2/4, c2*2

 G Major scale (the sharp version of a note is played if the note is preceeded by '#')
 G A B c d e #f g 

 F Major scale (flat version of a note is played if the note is preceeded by '~')
 (Sharp and flat versions are defined for each base note.  For #f or ~g are possible and will result in the same tone)
 F G A ~B c d e f

 F Major scale at double speed with detached notes and transposition
 f1/2, g1/2, a1/2, ~b1/2, c2/2, d2/2, e2/2, f2/2 
 
 C Major scale played faster (base speed is 120 bpm (duration of a note is 500ms), this will set the speed to 200bpm (duration 300ms)
 !=200 c d e f g a b c1

 C Major scale over two octaves played faster(at 150bpm), second octave played even faster at 300 bpm
 !=150 c d e f g a b c1 !=300 d1 e1 f1 g1 a1 b1 c2*2 

 Same as before, but with relative speed setting
 It is good practice to define the base speed at the beginning and later change only relative to the 
 base speed.
 In this example the base speed is 150 bpm
 !/2 will double that speed to 300 bpm (this will cut the length of a tone by 2)
 !*2 will half that speed to 75 bpm (this will double the length of a tone)
   (combining is possible. !*2/3 (or !/3*2) will change the length of a tone to 2/3rd, resulting in 225 bpm base length)
 ! would reset the speed to 150 bpm
 Any change of speed using ! will be valid for all notes follwing (till the next speed change with ! is given)
 !=150 c d e f g a b c1 !/2 d1 e1 f1 g1 a1 b1 c2*2 
 
 Second octave now played with length of notes increased by 1.5
 !=150 c d e f g a b c1 !/2*3 d1 e1 f1 g1 a1 b1 c2*2 

 First octave speed is doubled in the middle, but then set back to "normal" for second octave
 !=150 c d e !/2 f g a b c1 ! d1 e1 f1 g1 a1 b1 c2*2 

 First octave speed is doubled in the middle, but then set back to "system default" 120 bpm for second octave
 !=150 c d e !/2 f g a b c1 !. d1 e1 f1 g1 a1 b1 c2*2 

 Same as before, but second octave is pitched one octave higher by "octave shift"
 !=150 c d e !/2 f g a b c1 !. +1 d e f g a b c1*2 

 Same as before, but overall two octaves lower
 !=150 -2 c d e !/2 f g a b c1 !. -1 d e f g a b c1*2 

 Octave shifts are limited in the range of -4 to +4, higher/lower numbers are ignored
 Lowest note thus is -4 ~C the highest is +4 #b (which is in fact enharmonic +5 c)
 so in -5 ~C the -5 will be completely ignored, ~C will play
 -4 ~C is fine and will result in fast clicking noise at 7.7 Hz
 so in +5 #b the +5 will be ignored, #b will play
 in adding the octave shift to the transposition of the note self, any resulting shift >4 will be cut at 4
 i.e. with +3 b b1 b2 b3  the resulting playback will be b3 b4 b4 b4
 
 Unknown characters are simply ignored, so you can use | to divide "measures" for increasing the readeability
 !=200 c d e f | g*2, g*2, | a. a. a. a. | g*4, | a. a. a. a. | g*4, | f, f, f, f, | e*2, e*2, | g, g, g, g, | c*4

 Same as before, but no extra characters (most condensed version) 
 !=200cdefg*2,g*2,a.a.a.a.g*4,a.a.a.a.g*4,f,f,f,f,e*2,e*2,g,g,g,g,c*4

 A more involved example from the classical genre: the first measures of Thais Meditation from Massenet (total length ~22 seconds)
 (whith lots of spaces and measure bars to increase readeability)
 !=88 | !/2 #f*5 d !/3 A d #f | ! b*2 #c1 d1 |  !/2 d*3 e !/5 #fg#fe#f !/2 a   A  |6 ! B*3, !/2 #cd, | #fe ! g*2, !/2 #de, | #fg, a, b, ! b B, | #c*2 d !/4 ed#cd | ! e*2 f*2 | #f*3, 

 A more fun sample from the 80s: the first seconds of "Vamos a la playa" from Righeira
 !=600ar+1arar+0ar!*2ar!a1r!*2ar+1!erdrcrdr+0ar!*2ar!ar+1arar+0ar!*2ar!a1r!*2ar+1!erdrcr!*3e.!r!*3c.!r+!*3ar

 Or more heavy: intro of "Smoke on the water"
 !=300e*2,g*2,a*2,rerg*2~ba*3re*2g*2a*3.g*2e*4.r

 Or again back to the 80s: "Axel F" from Harold Faltermeyer
 !=130 f. !/4 ~a*3.f*2,f !/2 ~b,f,~e, | ! f. !/4  c1*3.f*2,f !/2 ~d1,c1,~a | fc1f1 !/4 f~e*2.~e, !/2 c, g, f*4,

 Film music: "Raiders March" from Indiana Jones
 !=125 !/4 e*3,f | ! g, c1*2, !/4 d*3,e | ! f*3. !/4 g*3,a | ! b, f1*2. !/4 a*3,b | ! c1 d1 e1 !/4 e*3, f |9 ! g c1*2 !/4 d1*3, e1 | ! f1*3, !/4 g*3,g | e1*4 d1*3, g e1*4 d1*3, g | e1*4 d1*3, g ! e1, d1, c1

 Same as above, condensed version:
 !=125!/4e*3,f!g,c1*2,!/4d*3,e!f*3.!/4g*3,a!b,f1*2.!/4a*3,b!c1d1e1!/4e*3,f!gc1*2!/4d1*3,e1!f1*3,!/4g*3,ge1*4d1*3,ge1*4d1*3,ge1*4d1*3,g!+1e,d,c

 Melodies can be turned into "alarm sounds" by increasing the speed. Here the same "Raiders March" at 5000 bpm (40 times faster)
 (Maximum possible bpm is 6000)
 !=5000!/4e*3,f!g,c1*2,!/4d*3,e!f*3.!/4g*3,a!b,f1*2.!/4a*3,b!c1d1e1!/4e*3,f!gc1*2!/4d1*3,e1!f1*3,!/4g*3,ge1*4d1*3,ge1*4d1*3,ge1*4d1*3,g!+1e,d,c

 If a melody starts with the sequence ":number" with number being a valid decimal number > 0, the melody will be repeated that number
 of times. There can be leading spaces before the ':'-sign but no other (printable) characters. No additional character (also not a 
 whitespace) is permitted between the ':' and the first digit of the following number, otherwise the melody will not be repeated.
 :5:P!=5000!/4e*3,f!g,c1*2,!/4d*3,e!f*3.!/4g*3,a!b,f1*2.!/4a*3,b!c1d1e1!/4e*3,f!gc1*2!/4d1*3,e1!f1*3,!/4g*3,ge1*4d1*3,ge1*4d1*3,ge1*4d1*3,g!+1e,d,c

 Enough for now, time for your own experiments!
 
 */
