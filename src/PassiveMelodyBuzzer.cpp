#include "PassiveMelodyBuzzer.h"
#if defined(ESP32)
#define NUM_TIMERS  4
#define TIMER_IDS {0, 1, 2, 3}
#elif defined(ESP8266)
#define NUM_TIMERS  1
#define TIMER_IDS {1}
#else
#error This Platform is not (yet) supported by the library!
#endif

const uint8_t timer_ids[NUM_TIMERS] = TIMER_IDS;
bool timer_alloc[NUM_TIMERS];


struct timerData_t {
    bool value, autostop, silence;
    long countDown;
    uint8_t pin;
#if defined(ESP8266)
    uint16_t pinMask;
#endif
};

volatile timerData_t timerData[4];

#if defined(ESP32)
static void IRAM_ATTR onTimer(int id) 
{
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
//    if (0 == tData->tick)
    {
//      tData->tick = 10;
      if (!tData->silence)
        {
        tData->value = !tData->value;
        digitalWrite(tData->pin, tData->value); 
        }
    }
//    tData->tick = tData->tick -1;
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
#elif defined(ESP8266)

#include <user_interface.h>
// https://www.esp8266.com/viewtopic.php?p=34411
#define TIMER1_DIVIDE_BY_1              0x0000
#define TIMER1_DIVIDE_BY_16             0x0004
#define TIMER1_DIVIDE_BY_256            0x0008

#define TIMER1_AUTO_LOAD                0x0040
#define TIMER1_ENABLE_TIMER             0x0080
#define TIMER1_FLAGS_MASK               0x00cc

#define TIMER1_NMI                      0x8000

#define TIMER1_COUNT_MASK               0x007fffff        // 23 bit timer

void IRAM_ATTR interruptHandler() {
volatile timerData_t *tData = &timerData[0];    
    if (tData->countDown)
    {
      if (0 == (tData->countDown = tData->countDown -1))
        if (tData->autostop)
          {
//            tData->value = false;
            pinMode(tData->pin, INPUT);
            digitalWrite(tData->pin, LOW);
            return;
          }
    }
    else if (tData->autostop)
        return;
//    if (0 == tData->tick)
    {
//      tData->tick = 10;
      if (!tData->silence)
        {
        if (16 == tData->pin)    
        {
//        pinMode(16, OUTPUT);
//        digitalWrite(16, !digitalRead(16));
//        pinMode(4, OUTPUT);
//        digitalWrite(4, !digitalRead(4));
//        return;    

            if (GP16I & 1)
                GP16O &= ~1;
            else
                GP16O |= 1;
        }
        else
            if (GPIP(tData->pin))
                GPOC = tData->pinMask;
            else
                GPOS = tData->pinMask;
        }
    }
}


#endif


void PassiveBuzzer::lowLevelStop(){
    pinMode(_pin, INPUT);
#if defined(ESP32)
    timerAlarmDisable(_timer);
#elif defined(ESP8266)   
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
//    Serial.println("ESP8266 Interrupt disabled!");
#endif
    digitalWrite(_pin, !_highActive);
    pinMode(_pin, OUTPUT);
}

void PassiveBuzzer::loadTimer(long frequencyDeciHz) {
#if defined(ESP32)
    timerAlarmWrite(_timer, 100000000l / frequencyDeciHz, true);
    timerAlarmEnable(_timer);
#elif defined(ESP8266)
    RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
    RTC_REG_WRITE(FRC1_LOAD_ADDRESS, (25000000 * 16 / frequencyDeciHz) & TIMER1_COUNT_MASK);
    ETS_FRC_TIMER1_INTR_ATTACH((void (*)(void *))interruptHandler, NULL);
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, TIMER1_FLAGS_MASK & (TIMER1_DIVIDE_BY_1 | TIMER1_AUTO_LOAD | TIMER1_ENABLE_TIMER));
#endif    
}


uint8_t PassiveBuzzer::_size = 0;
 
PassiveBuzzer * PassiveBuzzer::getBuzzer(int pin, bool highActive, uint8_t timerId)
{
uint8_t idx;    
#if defined(ESP32)
    if (pin >= 0)
#elif defined(ESP8266)
    if ((pin >= 0) && (pin <= 16))
#endif
        if (255 != (idx = allocateTimer(timerId)))
            return (new PassiveBuzzer(pin, highActive, idx));
    return NULL;
}

uint8_t PassiveBuzzer::allocateTimer(uint8_t timerId)
{
int i;
uint8_t ret = 255;
    if (_size < NUM_TIMERS)
        if (255 == timerId) 
        {
            for(int i = 0;i < NUM_TIMERS;i++)
                if (!timer_alloc[i])
                    break;
            if (i < NUM_TIMERS)
                ret = i;
        }
        else 
        {
            for (i = 0;i < NUM_TIMERS;i++)
                if (timer_ids[i] == timerId)
                    break;
            if ((i < NUM_TIMERS) && !timer_alloc[i])
                ret = i;
        }
    if (ret != 255)
    {
        timer_alloc[ret] = true;
        _size++;
    }
    return ret;
}

PassiveBuzzer::PassiveBuzzer(int pin, bool highActive, uint8_t id)
{
    _highActive = highActive;
//    if ((pin >= 0) && (_size < 3))
    {
        _pin = pin;
        //digitalWrite(pin, LOW);
        pinMode(pin, INPUT);
        _id = id;
        timerData[_id].value = false;
        timerData[_id].autostop = true;
//        timerData[_id].tick = 10;
        timerData[_id].pin = pin;
        timerData[_id].countDown = 0;
        timerData[_id].silence = false;
#if defined(ESP32)
        _timer = NULL;
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
#elif defined(ESP8266)
        Serial.printf("ESP8266 timer allocated with idx=%d, pin =%d\r\n", _id, pin);
        timerData[_id].pinMask = 1 << pin; 
#endif
        lowLevelStop();
    }
} 

void PassiveBuzzer::tone(uint32_t frequencyDeciHz, uint32_t durationCentiSec, bool autoStop){
    //timerAlarmDisable(_timer);
    //digitalWrite(_pin, LOW);
    //pinMode(_pin, OUTPUT);
    unsigned long long countdown = 0ULL;
    lowLevelStop();

    timerData[_id].value = LOW;
//    timerData[_id].tick = 10;
    timerData[_id].silence = false;
    timerData[_id].autostop = autoStop;
    countdown = (unsigned long long)durationCentiSec * (unsigned long long)frequencyDeciHz / 5000ULL;
    timerData[_id].countDown = (uint32_t)countdown;
/*    timerData[_id].countDown = durationCentiSec * frequencyDeciHz;
    timerData[_id].countDown /= 5000ul; 
    Serial.printf("Calculation %ld * % ld = (%ld / %ld) = %ld\r\n", 
    durationCentiSec, frequencyDeciHz, durationCentiSec* frequencyDeciHz, 5000l, 
               ((durationCentiSec* frequencyDeciHz) / 5000l));
*/
    //Serial.printf("CountDown: %ld\r\n", timerData[_id].countDown);
    if (1 > timerData[_id].countDown)
        timerData[_id].countDown = 1;
//    Serial.printf("Timer alarm enable for %ld\r\n", _timer);

//    timerAlarmWrite(_timer, 100000000l / frequencyDeciHz, true);
//    timerAlarmEnable(_timer);
    //delay(durationMs);
    loadTimer(frequencyDeciHz);
}

void PassiveBuzzer::click(){
/*    
    pinMode(_pin, INPUT);
#if defined(ESP32)    
    timerAlarmDisable(_timer);
#elif defined(ESP8266)
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
#endif
    digitalWrite(_pin, _highActive);
    pinMode(_pin, OUTPUT);
*/
    lowLevelStop();
    timerData[_id].value = HIGH;
//    timerData[_id].tick = 10;
    timerData[_id].silence = false;
    timerData[_id].autostop = true;
    timerData[_id].countDown = 5; 

    loadTimer(50000);
//    timerAlarmWrite(_timer, 100000000l / 50000, true);
//    timerAlarmEnable(_timer);
    //delay(durationMs);
}

void PassiveBuzzer::pause(uint32_t durationCentiSec) {
/*    
    pinMode(_pin, INPUT);
#if defined(ESP32)    
    timerAlarmDisable(_timer);
#elif defined(ESP8266)
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
#endif
    digitalWrite(_pin, LOW);
*/
    lowLevelStop();
    timerData[_id].value = LOW;
//    timerData[_id].tick = 10;
    timerData[_id].silence = true;
    timerData[_id].autostop = true;
    timerData[_id].countDown = durationCentiSec;
    if (1 > timerData[_id].countDown)
        timerData[_id].countDown = 1;
//    timerAlarmWrite(_timer, 100000000l / 5000, true);
//    timerAlarmEnable(_timer);
    loadTimer(5000);
} 

void PassiveBuzzer::stop() {
/*
    pinMode(_pin, INPUT);
#if defined(ESP32)
    timerAlarmDisable(_timer);
#elif defined(ESP8266)
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
#endif
    digitalWrite(_pin, LOW);
*/
    lowLevelStop();
    pinMode(_pin, INPUT);

    timerData[_id].value = LOW;
//    timerData[_id].tick = 10;
    timerData[_id].silence = true;
    timerData[_id].autostop = true;
    timerData[_id].countDown = 0;
} 


bool PassiveBuzzer::busy() {
#if defined(ESP32) || defined(ESP8266)
    yield();
#endif
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
    stop();
    
    //resetParams();
    /*
    _duration=60;
    _scanPause = 0;
    */
}

PassiveMelodyBuzzer::PassiveMelodyBuzzer()
{

    _myBuzzer = NULL;

    _melody = NULL;_nextTone= NULL;
    stop();
    
    //resetParams();
    /*
    _duration=60;
    _scanPause = 0;
    */
}
bool PassiveMelodyBuzzer::begin(int8_t pin, bool highActive)
{

    if (!_myBuzzer)
    _myBuzzer = PassiveBuzzer::getBuzzer(pin, highActive);

    stop();
    
    return _myBuzzer != NULL;
    //resetParams();
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
    _verbose = verbose;
    stop();
    if (melody)
    {
        while ((*melody > 0) && (*melody <= ' '))
            melody++;
        _debugLength = strlen(melody);
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
                _repeat = 0;
    }
}


void PassiveMelodyBuzzer::playMelody(String melody, uint8_t verbose)
{
    playMelody(melody.c_str(), verbose);
}

//20+1ce,e,e,e,c.c.c.c.e,e,e,c/2pPe.e.e,e,c,c.c.c.e,d,c,c,Pe/2d/3pPf/2e/3
const char * PassiveMelodyBuzzer::playTone(const char *melodyPart)
{
    bool scanSuccess, abort;
    uint32_t toneFreq, toneDuration, scanPause;
    if (!_myBuzzer)
    {
        stop();
        return NULL;
    }
//    Serial.printf("Starting scanTone with '%s'\r\n", melodyPart);
    //const char *ret = scanTone(melodyPart);
    const char* ret = scanTone(melodyPart, toneFreq, toneDuration, scanPause, scanSuccess, abort);
    _scanPause = scanPause;

    //if (!_scanSuccess)
    if (!scanSuccess)
    {
        if (abort)
            _repeat = 0;
        if (0 == _repeat)
        {
            if (_verbose)
            {
                Serial.printf("Done after %d tones. (Input length was %d bytes%s)\r\n\r\n",
                            _debugCount, _debugLength + 1,
                            abort?", Melody was aborted":"");
            } 
            stop();
        }
        return NULL;
    }
//    Serial.printf("Freq: %ld (Pause: %d), Duration: %ld\r\n", _scanFreq, _scanIsPause, _scanDuration);
    if (_verbose)
        Serial.printf("%4ld: Frequency: %4ld.%ldHz%s, Duration: %5ldms (+ %4ldms rest, total duration: %5ldms)\r\n", 
            ++_debugCount, toneFreq / 10, toneFreq % 10, (0 == toneFreq?"  (Rest)":" (Sound)"), toneDuration,
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
        _rest0 = DEFAULT_REST0;
        _rest1 = DEFAULT_REST1;
        _rest2 = DEFAULT_REST2;
        _rest3 = DEFAULT_REST3;
        _rest4 = DEFAULT_REST4;
        _durationOverride = 0;
        _restChar = _restCharOverride = 0;
        _div = 1;
        _mul = 1;
        _octave = 0;
        _nextTone = NULL;
}

void PassiveMelodyBuzzer::stop()
{
    if (_myBuzzer)
    {
        _myBuzzer->stop();
    //    Serial.println("STOP called for Buzzer!");
/*
        _scanPause = 0;
        _nextTone = NULL;
        _octave = 0;
        _duration = 50;
        _speedup = 1;
*/        
        resetParams();
        _debugCount = _debugLength = 0;
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
/*    
    if ('.' == *p)
    {
        x = 60000 / DEFAULT_BPM;
        return p;
    }
*/
    p = getNumber(p, x);
    if (x > 6000)
        x = 6000;
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

const char* getLength(const char* in, uint32_t& result)
{
const char* ret = in + 1;

char lChar = *in;
    if (('l' != lChar) && ('L' != lChar))
        return NULL;
    result = (lChar == tolower(lChar)?100:1000);
    in++;
    if (isDigit(*in))
    {
        uint32_t x;
        ret = getNumber(in, x) + 1;
        if (x > 0)
            result = x * result; 
    }
    return ret;
}

bool getRestChar(char c, char& out) {
    if (('.' == c) || (',' == c) || (';' == c) || (':' == c) || ('n' == c) || ('N' == c) || ('\'' == c))
    {
        if (isAlpha(c))
            out = 0;
        else 
            out = c;
        return true;
    } 
    return false;
}

const char* PassiveMelodyBuzzer::scanTone(const char* toneString, uint32_t & toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool &scanSuccess, bool &abort)
{
    char tone = 0;
    int sign = 0;
    bool prefixDone = false;
    bool directLength = false;
    char restChar;
    abort = false;
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
                uint32_t x;bool setOverride = false;char restCharx;bool setRelative = false;
                //bool restReset = false;
                _div = _mul = 1;
                const char *p = NULL;
/*                
                if ('.' == toneString[1])
                {
                    toneString++;
                    _duration = 60000 / DEFAULT_BPM;
                    _restDefault = _restChar = 0;
                }
                else 
*/
                if ('=' == toneString[1])
                {
                    toneString++;
                    p = getDuration(toneString + 1, x);
//                    Serial.println("Scanning new duration.");
                    _div = _mul = 1;
                    _restChar = _restDefault = 0;
                    if (NULL != p)
                    {
                        toneString = p;
                        _duration = x>0?x:60000/DEFAULT_BPM;

//                        Serial.printf("New duration: %ld\r\n", _duration);
                    }
                    else    
                        _duration = 60000/DEFAULT_BPM;
                }
                else if (setOverride = ('!' == toneString[1]))
                {
                    toneString++;
                    p = getDuration(toneString + 1, x);
//                    Serial.println("Scanning new duration override.");
                    if (NULL != p)
                    {
                        toneString = p;
                        _div = _mul = 1;
                        _restCharOverride = 0;
                        _durationOverride = x;
//                        Serial.printf("New duration override: %ld\r\n", _durationOverride);
                    }
                    else 
//                        Serial.println("FAIL!")
                        ;
                }
                else if (setRelative = (('*' == toneString[1]) || ('/' == toneString[1]))) 
                {
                        toneString = getMulDiv(toneString + 1, _mul, _div) - 1;
//                        _restChar = _restDefault;
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
                                    _div = _div * x;::scanTone
//                            if ('*' == p[1] || ('/' == p[1]))
                                toneString = p;
//                            else
//                                toneString = p;
                        }
                    }
*/
//                    Serial.printf("Resulting multiplier are MUL = %ld, DIV=%ld\r\n", _mul, _div);
                }
                else if (('l' == toneString[1]) || ('L' == toneString[1]))
                {
                    uint32_t x;
                    toneString++;
//                    Serial.printf("Get length direct = %s\r\n", toneString);
                    if (NULL != (p = getLength(toneString, x)))
                    {
                        toneString = p - 1;
                        _duration = x;
//                        Serial.printf("Duration is now: %ld\r\n", _duration);
                    }
                }
                else                // Special case: ! w/o valid next char
                {
//                    Serial.println("Special set relative Empty!!!");
                    setRelative = true;
//                    _restChar = _restDefault;
                    //restReset = true;
                }
//                if (!setRelative && !setOverride)
                if (!setOverride)
                {
//                    if (!restReset)
//                        _restDefault = 0;
                    _restChar = _restDefault;
                }
                if (getRestChar(*(toneString + 1), restCharx))
                {
                    if (setOverride)
                        _restCharOverride = restCharx;
                    if (!setRelative)
                        _restChar = _restDefault = restCharx;
                    else
                        _restChar = restCharx;
                    toneString++;
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
            else if ('?' == tone)
            {
                if (getRestChar(*(++toneString), restChar))
                {
                    uint32_t x = 101;
                    if (isDigit(*(toneString + 1)))
                        toneString = getNumber(toneString + 1, x);
                    if ('\'' == restChar)
                        _rest0 = (x < 101)?x:DEFAULT_REST0;
                    else if (',' == restChar)
                        _rest1 = (x < 101)?x:DEFAULT_REST1;
                    else if ('.' == restChar)
                        _rest2 = (x < 101)?x:DEFAULT_REST2;
                    else if (';' == restChar)
                        _rest3 = (x < 101)?x:DEFAULT_REST3;
                    else 
                        _rest4 = (x < 101)?x:DEFAULT_REST4;
                }
                else
                {
                    _rest0 = DEFAULT_REST0;
                    _rest1 = DEFAULT_REST1;
                    _rest2 = DEFAULT_REST2;
                    _rest3 = DEFAULT_REST3;
                    _rest4 = DEFAULT_REST4;
                }
            }
            else if (abort = ('x' == tone))
                return NULL;
        }
        toneString++;
    }
    if (!prefixDone)
    {
//        Serial.println("ScanTone Failed.");
        return NULL;
    }
    scanSuccess = true;
    char givenTone = *(toneString - 1);
    toneDuration = (_durationOverride != 0)?_durationOverride:_duration;
    restChar = (_durationOverride != 0)?_restCharOverride:_restChar;
    if (_div * _mul != 1)
        toneDuration = toneDuration * _mul / _div;
    if (toneDuration < 1)
        toneDuration = 1;


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
    if (('*' == *toneString) || ('/' == *toneString)
             || ('l' == *toneString) || ('L' == *toneString))
            
    {
//        Serial.printf("Lengt specified: %s\r\n", toneString);Serial.flush();
        if (isalpha(*toneString))
        {
            toneString = getLength(toneString, toneDuration);
            /*
//            Serial.printf("Direct length='%s'\r\n", toneString + 1);Serial.flush();
            uint32_t x;
            uint32_t mul = ((*toneString == 'L')?1000:100);
            toneString = getNumber(toneString + 1, x);
            if (x < 1)
                x = 1;
            toneDuration = x * mul;
            toneString++;
        */
        }    
        else
        {
            uint32_t mul, div;
            if ((tone == 'r') && (givenTone == 'R'))
                toneDuration = toneDuration / 2;
            toneString = getMulDiv(toneString, mul, div);
            if (1 != mul * div)
                toneDuration = toneDuration * mul / div;
            if (toneDuration < 1)
                toneDuration = 1;
        }
    }
//    Serial.printf("Tone duration: %ld\r\nToneString=%s\r\n", toneDuration, toneString);Serial.flush();
    if (getRestChar(*toneString, restChar))
    {
        if (0 == toneFreq)
            restChar = 0;
        toneString++;
    }
//    if ((('.' == *toneString) || (',' == *toneString)) && (0 != toneFreq))
    if (restChar)
        restChar = (',' == restChar?_rest1:('.' == restChar?_rest2:(';' == restChar?_rest3:
                    ('\'' == restChar?_rest0:_rest4))));
    if (restChar)
        {
//            tonePause = toneDuration * ('.' == restChar?4:1);
            tonePause = toneDuration * restChar;
            tonePause = tonePause / 100;
            if (tonePause < toneDuration)
                toneDuration -= tonePause;
            else
                tonePause = 0;
//            toneString++;
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
        }  buzzer = new PassiveMelodyBuzzer(BUZZER_PIN);

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
