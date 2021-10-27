#ifndef _PASSIVE_BUZZER_H
#define _PASSIVE_BUZZER_H
#include <Arduino.h>

#define DEFAULT_BPM 120

class PassiveBuzzer 
{
public:
    static PassiveBuzzer *getBuzzer(int pin, bool highActive = true);
    void tone(long frequencyDeciHz, long durationCentiSec, bool autoStop=true);
    void pause(long durationCentiSec);
    void click();
    void stop();
    bool busy();
    uint32_t busyCount();
    static uint8_t allocateTimer();
    uint8_t getId() {return _id;};
private:        
    static uint8_t _size;
    bool _highActive;
    PassiveBuzzer(int pin, bool highActive); 
    uint8_t _pin, _id;
    hw_timer_t* _timer = NULL;
};

class PassiveMelodyBuzzer
{
public:
    PassiveMelodyBuzzer(int8_t pin, bool highAvtive = true);
    PassiveMelodyBuzzer();
    bool begin(int8_t pin, bool highAvtive = true);
    bool busy();
    inline bool loop() {return busy();};
    void stop();
    const char *playTone(const char *melodyPart);
    void playMelody(const char *melody, uint8_t verbose = 0);
    void playMelody(String melody, uint8_t verbose = 0);
    void click();
    uint32_t busyCount() {return _myBuzzer?_myBuzzer->busyCount():0;};
    uint8_t getId() {if (_myBuzzer) return _myBuzzer->getId(); return 255;};
protected:
    const char* scanTone(const char*, uint32_t& toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool& scanSuccess);
    void resetParams();
    PassiveBuzzer* _myBuzzer;    
    int _playMelodyState;
    int _repeat;
    const char * _melody;
    const char *_nextTone;
    uint32_t _scanPause;
    uint32_t _duration; 
    uint32_t _durationOverride;
    uint32_t _div;
    uint32_t _mul;
    int8_t _octave;
    uint8_t _verbose;

    uint16_t _debugCount, _debugLength;


};

#endif