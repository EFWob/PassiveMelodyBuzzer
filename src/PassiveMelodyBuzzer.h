#ifndef _PASSIVE_BUZZER_H
#define _PASSIVE_BUZZER_H
#include <Arduino.h>

#define DEFAULT_BPM 120
#define DEFAULT_REST0   10
#define DEFAULT_REST1   25
#define DEFAULT_REST2   37
#define DEFAULT_REST3   50
#define DEFAULT_REST4   75

class PassiveBuzzer;

class PassiveMelodyBuzzer
{
public:
    PassiveMelodyBuzzer(int8_t pin, bool highActive = true, uint8_t timerID = 0xff);
    PassiveMelodyBuzzer();
    bool begin(int8_t pin = -1, bool highAvtive = true, uint8_t timerId = 0xff);
    bool busy();
    inline bool loop() {return busy();};
    void stop();
    void playMelody(const char *melody, uint8_t verbose = 0);
    void playMelody(String melody, uint8_t verbose = 0);
    void playRTTTL(const char* melody, uint8_t verbose);
    void click();
    uint32_t busyCount();
    uint8_t getId();
private:
    const char* scanTone(const char*, uint32_t& toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool& scanSuccess, bool& abort);
    const char* scanToneRTTTL(const char*, uint32_t& toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool& scanSuccess, bool& abort);
    const char *playTone(const char *melodyPart);
    void resetParams();
    PassiveBuzzer* _myBuzzer;   
    int _repeat;
    uint32_t _repeatTimeout, _repeatStart;
    const char * _melody;
    const char *_nextTone;
    uint32_t _scanPause;
    uint32_t _duration; 
    uint32_t _durationOverride;
    uint8_t _rest0, _rest1, _rest2, _rest3, _rest4;
    char _restChar, _restCharOverride, _restDefault;
    uint32_t _div;
    uint32_t _mul;
    int8_t _octave;
    uint8_t _verbose;
    uint16_t _RTTTLd;

    uint16_t _debugCount, _debugLength;


};

#endif