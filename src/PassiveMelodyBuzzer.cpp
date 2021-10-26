#include "PassiveMelodyBuzzer.h"

struct timerData_t {
    bool value, autostop, silence;
    uint32_t countDown;
    uint8_t tick, pin;

};

volatile timerData_t timerData[4];

static void IRAM_ATTR onTimer(int id) {
volatile timerData_t *tData = &timerData[id];    
    if (tData->countDown)
    {
      if (0 == (tData->countDown = tData->countDown -1))
        if (tData->autostop)
          {
            tData->value = false;
            pinMode(tData->pin, INPUT);
            digitalWrite(tData->pin, LOW);
            return;
          }
    }
    else if (tData->autostop)
        return;
    if (0 == tData->tick)
    {
      tData->tick = 10;
      if (!tData->silence)
        {
        tData->value = !tData->value;
        digitalWrite(tData->pin, tData->value); 
        }
    }
    tData->tick = tData->tick -1;
}

static void IRAM_ATTR onTimer0() {
    onTimer(0);
}

static void IRAM_ATTR onTimer1() {
    onTimer(1);
}

static void IRAM_ATTR onTimer2() {
    onTimer(2);
}

static void IRAM_ATTR onTimer3() {
    onTimer(3);
}


uint8_t PassiveBuzzer::_size = 0;
 
PassiveBuzzer * PassiveBuzzer::getBuzzer(int pin, bool highActive)
{
    
    if (pin >= 0)
        if (255 != allocateTimer())
            return (new PassiveBuzzer(pin, highActive));
    return NULL;
}

uint8_t PassiveBuzzer::allocateTimer()
{
uint8_t ret = 255;
    if (_size < 3)
        return _size++;
    return ret;
}

PassiveBuzzer::PassiveBuzzer(int pin, bool highActive)
{
    _highActive = highActive;
    _timer = NULL;
//    if ((pin >= 0) && (_size < 3))
    {
        _pin = pin;
        //digitalWrite(pin, LOW);
        pinMode(pin, INPUT);
        _id = _size - 1;
        timerData[_id].value = false;
        timerData[_id].autostop = true;
        timerData[_id].tick = 10;
        timerData[_id].pin = pin;
        timerData[_id].countDown = 0;
        timerData[_id].silence = false;
        _timer = timerBegin(_id, 4, true);//div 80
        timerAlarmDisable(_timer);
        if (_id == 0)
            timerAttachInterrupt(_timer, &onTimer0, true);
        else if (_id == 1)
            timerAttachInterrupt(_timer, &onTimer1, true);
        else if (_id == 2)
            timerAttachInterrupt(_timer, &onTimer2, true);
        else if (_id == 3)
            timerAttachInterrupt(_timer, &onTimer3, true);
        //_size++;
    }
} 

void PassiveBuzzer::tone(long frequencyDeciHz, long durationCentiSec, bool autoStop){
    timerAlarmDisable(_timer);
    digitalWrite(_pin, LOW);
    pinMode(_pin, OUTPUT);
    timerData[_id].value = LOW;
    timerData[_id].tick = 10;
    timerData[_id].silence = false;
    timerData[_id].autostop = autoStop;
    timerAlarmWrite(_timer, 10000000l / frequencyDeciHz, true);
    timerData[_id].countDown = durationCentiSec * frequencyDeciHz / 500; 
    if (1 > timerData[_id].countDown)
        timerData[_id].countDown = 1;
//    Serial.printf("Timer alarm enable for %ld\r\n", _timer);
    timerAlarmEnable(_timer);
    //delay(durationMs);
}

void PassiveBuzzer::click(){
    timerAlarmDisable(_timer);
    digitalWrite(_pin, _highActive);
    pinMode(_pin, OUTPUT);
    timerData[_id].value = HIGH;
    timerData[_id].tick = 10;
    timerData[_id].silence = false;
    timerData[_id].autostop = true;
    timerAlarmWrite(_timer, 10000000l / 5000, true);
    timerData[_id].countDown = 5; 
    timerAlarmEnable(_timer);
    //delay(durationMs);
}

void PassiveBuzzer::pause(long durationCentiSec) {
    timerAlarmDisable(_timer);
    digitalWrite(_pin, LOW);
    timerData[_id].value = LOW;
    timerData[_id].tick = 10;
    timerData[_id].silence = true;
    timerData[_id].autostop = true;
    timerAlarmWrite(_timer, 10000000l / 500, true);
    timerData[_id].countDown = durationCentiSec;
    if (1 > timerData[_id].countDown)
        timerData[_id].countDown = 1;
    timerAlarmEnable(_timer);
} 

void PassiveBuzzer::stop() {
    timerAlarmDisable(_timer);
    pinMode(_pin, INPUT);
    digitalWrite(_pin, LOW);
    timerData[_id].value = LOW;
    timerData[_id].tick = 10;
    timerData[_id].silence = true;
    timerData[_id].autostop = true;
    timerData[_id].countDown = 0;

} 


bool PassiveBuzzer::busy() {
    return (0 != timerData[_id].countDown);
}

uint32_t PassiveBuzzer::busyCount() {
    return (timerData[_id].countDown);

}

struct toneItem_t
{
    long freq[3];
};

//40b-/3,b-/3pa,b-c,b-,g,b-/3,b-/3

toneItem_t tones[7] =
{
    //a, ais, as
    {2200, 2331, 2077},
    //h, his, b
    {2470, 2616, 2313},
    //c, cis, ces
    {1308, 1386, 1235},
    //d, dis, des
    {1468, 1556, 1386},
    //e, eis, es
    {1648, 1746, 1556},
    //f, fis, fes
    {1746, 1850, 1648},
    //g, gis, ges
    {1960, 2077, 1850}
};

PassiveMelodyBuzzer::PassiveMelodyBuzzer(int8_t pin, bool highActive)
{
    _myBuzzer = PassiveBuzzer::getBuzzer(pin, highActive);

    _melody = NULL;_nextTone= NULL;
    
    resetParams();
    /*
    _duration=60;
    _scanPause = 0;
    */
}

bool PassiveMelodyBuzzer::busy()
{
bool ret = false;
    if (_myBuzzer) {
        if (_myBuzzer->busy())
            return true;
        if (_scanPause)
            {
//                Serial.printf("Starting scanPause %ld\r\n", _scanPause);
                _myBuzzer->pause(_scanPause);
                _scanPause = 0;
                ret = true;
            }
        else if (_nextTone)
        {
            _nextTone = playTone(_nextTone);
            if (!_nextTone)
            {
//                Serial.printf("Seems to be done, have Repeat=%d\r\n", _repeat);
                if (_repeat)
                {
//                    Serial.printf("Start over with melody: %s\r\n", 
//                        _melody?_melody:"<null???>");
/*                    _octave = 0;
                    _duration = 50;
                    _speedup = 1;
*/
                    resetParams();
                    _nextTone = playTone(_melody);
                    --_repeat;
                    ret = true;
                }
                else
                {
                    _myBuzzer->stop();
                    if (_melody)
                    {
                        free((void *)_melody);
                        _melody = NULL;
                    }
                }
            }
            else
                ret = true;
        }
        else if (_melody)
        {
//            Serial.printf("Deleting Melody!");
            free ((void *)_melody);
            _melody = NULL;
        }
    }
    return ret;
}

void PassiveMelodyBuzzer::playMelody(const char *melody, uint8_t verbose)
{
    stop();
    _verbose = verbose;
    if (melody)
    {
        while ((*melody > 0) && (*melody <= ' '))
            melody++;
        _repeat = (*melody == ':')?1:0;
        if (_repeat)
        {
            melody++;
            if ((*melody >= '0') && (*melody <= '9'))
            {
                const char *p = melody  + 1;
                while ((*p >= '0') && (*p <= '9'))
                {
                    p++;
                }
                _repeat = atoi(melody);
                if (_repeat < 1)
                    _repeat = 1;
                if (*p == ':')
                    p++;
                melody = p;
            }
            _repeat = _repeat - 1;
        }
//        Serial.printf("IsRepeat: %d\r\n", _repeat);
        _melody = strdup(melody);
        if (_melody) 
            if (NULL == (_nextTone = playTone(_melody)))
                _repeat = false;
    }
}


void PassiveMelodyBuzzer::playMelody(String melody, uint8_t verbose)
{
    playMelody(melody.c_str(), verbose);
}

//20+1ce,e,e,e,c.c.c.c.e,e,e,c/2pPe.e.e,e,c,c.c.c.e,d,c,c,Pe/2d/3pPf/2e/3
const char * PassiveMelodyBuzzer::playTone(const char *melodyPart)
{
    bool scanSuccess;
    uint32_t toneFreq, toneDuration, scanPause;
    if (!_myBuzzer)
    {
        stop();
        return NULL;
    }
//    Serial.printf("Starting scanTone with '%s'\r\n", melodyPart);
    //const char *ret = scanTone(melodyPart);
    const char* ret = scanTone(melodyPart, toneFreq, toneDuration, scanPause, scanSuccess);_scanPause = scanPause;

    //if (!_scanSuccess)
    if (!scanSuccess)
    {
        if (0 == _repeat)
            stop();
        return NULL;
    }
//    Serial.printf("Freq: %ld (Pause: %d), Duration: %ld\r\n", _scanFreq, _scanIsPause, _scanDuration);
    if (_verbose)
        Serial.printf("Frequency: %4ld.%ldHz%s, Duration: %5ldms (+ %4ldms rest, total duration: %5ldms)\r\n", 
            toneFreq / 10, toneFreq % 10, (0 == toneFreq?"  (Rest)":" (Sound)"), toneDuration,
            scanPause, toneDuration + scanPause);
//    if (_scanIsPause)
    if (0 == toneFreq)
//        _myBuzzer->pause(_scanDuration);
        _myBuzzer->pause(toneDuration);
    else
//        _myBuzzer->tone(_scanFreq, _scanDuration, false, _speedup);
        _myBuzzer->tone(toneFreq, toneDuration, false);
    return ret;
}

void PassiveMelodyBuzzer::click() {
    if (_myBuzzer)
    {
        stop();
        _myBuzzer->click();
    }

}


void PassiveMelodyBuzzer::resetParams()
{
        _duration = 60000 / DEFAULT_BPM;
        _durationOverride = 0;
        _div = 1;
        _mul = 1;
        _octave = 0;
}

void PassiveMelodyBuzzer::stop()
{
    if (_myBuzzer)
    {
        _myBuzzer->stop();
//        Serial.println("STOP called for Buzzer!");
/*
        _scanPause = 0;
        _nextTone = NULL;
        _octave = 0;
        _duration = 50;
        _speedup = 1;
*/        
        resetParams();

        if (_melody)
        {
            free((void *)_melody);
            _melody = NULL;
        }
    }
}





const char* getNumber(const char* p, uint32_t& x)
{
const char* p1;
    if (!isdigit(*p))
        return NULL;
    p1 = p;
    while(isdigit(p1[1]))
        p1++;
    x = atoi(p);
    return p1;
}

const char* getDuration(const char* p, uint32_t& x)
{
    if ('.' == *p)
    {
        x = 60000 / DEFAULT_BPM;
        return p;
    }
    p = getNumber(p, x);
    if (x > 60000)
        x = 60000;
    else if (0 == x)
        x = DEFAULT_BPM;
    if (p)
        x = 60000 / x;
    return p;
}

const char* getFreq(const char* p, uint32_t x)
{
    const char* ret = getNumber(p, x);
    if (NULL == ret)  
        x = 4400;
    return ret;
}

const char* getMulDiv(const char* str, uint32_t& mul, uint32_t& div) 
{
    mul = div = 1;
    char c;
//    Serial.println("Adjusting duration multipliers...");
    while (('*' == *str) || ('/' == *str))
        {
            uint32_t x;
            const char *p = getNumber(str + 1, x);
//            if (p)
//                Serial.printf("After MulDiv remains: %s\r\n", p);
            if (p)
                {
                if (x > 0)
                    if ('*' == *str)
                        mul = mul * x;
                    else
                        div = div * x;
                    str = p + 1;
                }
            else
                str++;
        }
    return str;
}

const char* PassiveMelodyBuzzer::scanTone(const char* toneString, uint32_t & toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool &scanSuccess)
{
    char tone;
    int sign = 0;
    bool prefixDone = false;
//    _scanIsPause = false;
    tonePause = 0;
    scanSuccess = false;
    if (NULL == toneString)
        return NULL;
    while (!prefixDone && (0 != (tone = tolower(*toneString))))
    {
        if (!(prefixDone = ((tone == 'r') || (tone == '@') || ((tone >= 'a') && (tone <= 'g'))))) {
            if ('!' == tone)
            {
                uint32_t x;
                _div = _mul = 1;
                const char *p = NULL;
                if ('.' == toneString[1])
                {
                    toneString++;
                    _duration = 60000 / DEFAULT_BPM;
                }
                else if ('=' == toneString[1])
                {
                    toneString++;
                    p = getDuration(toneString + 1, x);
//                    Serial.println("Scanning new duration.");
                    if (NULL != p)
                    {
                        toneString = p;
                        _div = _mul = 1;
                        _duration = x>0?x:60000/DEFAULT_BPM;
//                        Serial.printf("New duration: %ld\r\n", _duration);
                    }
                    else    
//                        Serial.println("FAIL!")
                        ;
                }
                else if ('!' == toneString[1])
                {
                    toneString++;
                    p = getDuration(toneString + 1, x);
//                    Serial.println("Scanning new duration override.");
                    if (NULL != p)
                    {
                        toneString = p;
                        _div = _mul = 1;
                        _durationOverride = x;
//                        Serial.printf("New duration override: %ld\r\n", _durationOverride);
                    }
                    else 
//                        Serial.println("FAIL!")
                        ;
                }
                else if (('*' == toneString[1]) || ('/' == toneString[1]))
                {
                    toneString = getMulDiv(toneString + 1, _mul, _div) - 1;
/*
                    _div = _mul = 1;
                    char c;
                    Serial.println("Adjusting duration multipliers...");
                    while (('*' == toneString[1]) || ('/' == toneString[1]))
                    {
                        uint32_t x;
                        toneString++;
                        const char *p = getNumber(toneString + 1, x);
                        if (p)
                            Serial.printf("Afte MulDiv remains: %s\r\n", p);
                        if (p)
                        {
                            if (x > 0)
                                if ('*' == *toneString)
                                    _mul = _mul * x;
                                else
                                    _div = _div * x;
//                            if ('*' == p[1] || ('/' == p[1]))
                                toneString = p;
//                            else
//                                toneString = p;
                        }
                    }
*/
//                    Serial.printf("Resulting multiplier are MUL = %ld, DIV=%ld\r\n", _mul, _div);
                }
            }
            else if (('+' == tone) || ('-' == tone))
            {
                _octave = 0;
                if (isdigit(toneString[1]))
                    {
                    toneString++;
                    if ((*toneString > '0') && (*toneString < '5'))
                        _octave = (*toneString - '0') * (('+' == tone)?1:-1);
                    }
//                Serial.printf("Octave is now: %d\r\n", (int)_octave);
            } 
            else if (('#' == tone) || ('~' == tone))
            {
                char x = tolower(toneString[1]);
                if ((x >= 'a') && (x <= 'g'))
                    sign = (tone == '#'?1:2);
            }
        }
        toneString++;
    }
    if (!prefixDone)
    {
//        Serial.println("ScanTone Failed.");
        return NULL;
    }
    toneDuration = (_durationOverride != 0)?_durationOverride:_duration;
    if (_div * _mul != 1)
        toneDuration = toneDuration * _mul / _div;
    if (toneDuration < 1)
        toneDuration = 1;
    scanSuccess = true;
    char givenTone = *(toneString - 1);
//    Serial.printf("Tone is: '%c' (was '%c')\r\n", tone, givenTone);
    if ('r' == tone)
    {
        toneFreq = 0;
        if (tone != givenTone)
            toneDuration = toneDuration * 2;
        if (isdigit(*toneString))
            toneString++;
    }
    else if ('@' == tone)
    {
        toneFreq = 4400;
        if (isdigit(*toneString)) 
        {
            toneString = getNumber(toneString, toneFreq) + 1;
            if (0 == toneFreq)
                toneFreq = 4400;
        }
    } 
    else 
    {
        int octave = 0;
        /*
        int idx=('#' == *toneString)?1:(('~' == *toneString)?2:0);
        if (0 != idx)
            toneString++;
        */
        toneFreq = tones[tone - 'a'].freq[sign];
        if ((*toneString >= '1') && (*toneString <= '4'))
        {
            octave = *toneString - '0';
            toneString++;
        } 
        if (tone == givenTone)
            octave++;
        else
            octave = 0;
        octave = octave + _octave;
        if (octave > 0)
        {
            if (octave > 5)
                octave = 5;
            toneFreq = (toneFreq << octave);
        }
        else if (octave < 0)
        {
            if (octave < -4)
                octave = -4;
            toneFreq = (toneFreq >> -octave);
        }
        if (toneFreq < 1)
            toneFreq = 1;    
    }
    if (('*' == *toneString) || ('/' == *toneString))
    {
        uint32_t mul, div;
        if ((tone == 'r') && (givenTone == 'R'))
            toneDuration = toneDuration / 2;
        toneString = getMulDiv(toneString, mul, div);
        if (1 != mul * div)
            toneDuration = toneDuration * mul / div;
    }
    if ((('.' == *toneString) || (',' == *toneString)) && (0 != toneFreq))
        {
            tonePause = toneDuration * ('.' == *toneString?4:1);
            tonePause = tonePause / 10;
            if (tonePause < toneDuration)
                toneDuration -= tonePause;
            else
                tonePause = 0;
            toneString++;
        }
//    Serial.printf("ScanTone done, remaining String is: %s\r\n", toneString);
    return toneString;
}

/*
const char* PassiveMelodyBuzzer::scanTone1(const char* toneString)
{
char tone;    
uint8_t idx = 0;
long octave = 0;
    _scanIsPause = false;
    _scanSuccess = false;
    _scanDuration = 1;
    _scanPause=0;
    if (NULL == toneString)
        return NULL;
    while (*toneString && (NULL == strchr("abcdefg123456789p@+-(", tolower(*toneString))))
        toneString++;
    //Serial.printf("scanTone found: %d\r\n", *toneString);delay(1000);
    if (!*toneString)
        return NULL;
    tone = tolower(*toneString);
    if (tone == '(')
    {
        const char *p = toneString + 1;
        while ((*p >= '0') && (*p <= '9'))
            p++;
        _speedup = atoi(toneString + 1);
        if (_speedup < 1)
            _speedup = 1;
        //dbgprint("Speedup %ld (from %s)", _speedup, toneString + 1);
        while (*p && (NULL == strchr("abcdefg123456789p@+-", tolower(*p))))
            p++;
        toneString = p;
        tone = tolower(*p);
    }
    if ((tone >= '0') && (tone <= '9'))
    {
        const char *p = toneString + 1;
        while ((*p >= '0') && (*p <= '9'))
            p++;
        _duration = atoi(toneString);
        if (_duration == 0)
            _duration = 120;
        _duration = 60000 / _duration;
        if (_duration < 1)
            _duration = 1;
        while (*p && (NULL == strchr("abcdefgp@+-", tolower(*p))))
            p++;
        toneString = p;
        tone = tolower(*p);
    }
    if ((tone == '+') || (tone == '-'))
    {
        toneString++;
        if ((tone == '+') && (*toneString >= '0') && (*toneString <= '4'))
        {
            _octave = *toneString - '0';
            toneString++;
        }
        else if ((tone == '-') && (*toneString >= '0') && (*toneString <= '4'))
        {
            _octave = '0' - *toneString;
            toneString++;
        }
        else 
            _octave = 0;
        while (*toneString && (NULL == strchr("abcdefgp@", tolower(*toneString))))
            toneString++;
        tone = tolower(*toneString);
    }
    if (tone == '@')
    {
        toneString++;
        while (*toneString && (*toneString <= ' '))
            toneString++;
        if ((*toneString >= '0') && (*toneString <= '9'))
        {
            const char* p = toneString + 1;
            while ((*p >= '0') && (*p <= '9'))
                p++;
            _scanFreq=atoi(toneString);
            toneString = p;
            _scanSuccess = true;
        }
    }
    else if ((tone >= 'a') && (tone <= 'g') || (tone == 'p'))
    {
        if (tone == 'p')
        {
            _scanIsPause = true;
            if (tone != *toneString)
                _scanDuration = 2;
            tone = 'a';
        }
        else 
        {
            if (tone == *toneString)
                octave = 1;
        }
        _scanSuccess = true;
        toneString++;
        while (*toneString && (*toneString <= ' '))
            toneString++;
        if ((*toneString == '#') || (*toneString == '~'))
        {
            idx = ((*toneString == '#')?1:2);
            toneString++;
            while (*toneString && (*toneString <= ' '))
                toneString++;
        }
        _scanFreq = (tones[tone - 'a'].freq[idx]);
    }
    //Serial.printf("Tone: %c\r\n", tone);Serial.flush();delay(1000);
    if (!_scanSuccess)
        return NULL;
    //Serial.printf("Idx: %d\r\n", idx);Serial.flush();delay(1000);
    while (*toneString && (NULL == strchr("'/l.,abcdefgp@0123456789+-", tolower(*toneString))))
        *toneString++;
    if (*toneString == '\'')
    {
        toneString++;
        if ((*toneString >= '1') && (*toneString <= '4'))
        {
            octave = octave + *toneString - '0';
            toneString++;
        }
        while (*toneString && (NULL == strchr("/l.,abcdefgp@0123456789+-", tolower(*toneString))))
            toneString++;
    }
    if (_octave + octave > 5)
    {
        _octave = 5;
        octave = 0;
    }    
    if (_octave + octave >= 0)
        _scanFreq = (_scanFreq) << (_octave + octave);
    else
        _scanFreq = (_scanFreq) >> (-(_octave + octave));
    if ((*toneString == '/') || (*toneString == 'l') << (*toneString == 'L'))
    {
        toneString++;
        while (*toneString && (NULL == strchr("0123456789.,abcdefgp@+-", tolower(*toneString))))
            toneString++;
        if ((*toneString >= '0') && (*toneString <= '9'))
        {
            const char *p = toneString + 1;
            while ((*p >= '0') && (*p <= '9'))
                p++;
            _scanDuration = _scanDuration * atoi(toneString);
            while (*p && (NULL == strchr("abcdefgp@0123456789+-.,", tolower(*p))))
                p++;
            toneString = p;
        }
    }
    _scanDuration = _duration * _scanDuration;
    if ((*toneString == '.') || (*toneString == ','))
    {
        if (!_scanIsPause)
            if (*toneString == ',')
            {
                _scanPause = _duration/10;
                if (!_scanPause)
                    _scanPause = 1;
                //else if (_scanPause > 6)
                //    _scanPause = 6;
            } 
            else     
            {
                _scanPause = _duration/3;
                if (!_scanPause)
                    _scanPause = 1;
                //else if (_scanPause > 6)
                //    _scanPause = 6;
            }
        _scanDuration = _scanDuration - _scanPause;
        toneString++;
    }
    //Serial.println("ScanToneDone!");delay(1000);
    return toneString;
}

void PassiveMelodyBuzzer::playMelodyNew(const char* toneString)
{
    uint32_t toneFreq, toneDuration, tonePause;
    bool scanSuccess;
    _duration = 60000 / DEFAULT_BPM;
    _durationOverride = 0;
    _div = 1;
    _mul = 1;
    _octave = 0;
    scanToneNew(toneString, toneFreq, toneDuration, tonePause, scanSuccess);
    
    Serial.printf("ScanTone freq = %ld, duration = %ld\r\n", toneFreq, toneDuration);   
}

*/
