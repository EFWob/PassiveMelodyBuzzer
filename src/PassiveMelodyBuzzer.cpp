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

class PassiveBuzzer 
{
public:
    static PassiveBuzzer *getBuzzer(int pin, bool highActive = true, uint8_t timerId = 0xff);
    void pin(int pin, bool highActive);
    void tone(uint32_t frequencyDeciHz, uint32_t durationCentiSec, bool autoStop=true);
    void pause(uint32_t durationCentiSec);
    void click();
    void stop();
    bool busy();
    uint32_t busyCount();
    static uint8_t allocateTimer(uint8_t timerId = 0xff);
    uint8_t getId() {return _id;};
private:        
    static uint8_t _size;
    bool _highActive;
    PassiveBuzzer(int pin, bool highActive, uint8_t id); 
    void initTimer();
    void stopTimer();
    void loadTimer(long frequencyDeciHz);
    uint8_t _pin, _id;
#if defined(ESP32)
    hw_timer_t* _timer = NULL;
#endif
};



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


void PassiveBuzzer::initTimer(){
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
    timerData[_id].pinMask = 1 << _pin; 
#endif
    stop();
}


void PassiveBuzzer::stopTimer(){
    pinMode(_pin, INPUT);
#if defined(ESP32)
    timerAlarmDisable(_timer);
#elif defined(ESP8266)   
    ETS_FRC1_INTR_DISABLE();
    TM1_EDGE_INT_DISABLE();
    RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
#endif
    digitalWrite(_pin, !_highActive);
    pinMode(_pin, OUTPUT);
}

void PassiveBuzzer::loadTimer(long frequencyDeciHz) {
    if (1 > timerData[_id].countDown)
        timerData[_id].countDown = 1;
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
    if (pin >= 0)
#if defined(ESP32)
        if (pin <= 32)
#elif defined(ESP8266)
        if (pin <= 16)
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
    {
        if (255 == timerId) 
        {
            for(i = 0;i < NUM_TIMERS;i++)
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
    }
    if (ret != 255)
    {
        timer_alloc[ret] = true;
        _size++;
    }
    return ret;
}

void PassiveBuzzer::pin(int pin, bool highActive) {
    stop();
    pinMode(_pin = pin, INPUT);
    _highActive = highActive;
}

PassiveBuzzer::PassiveBuzzer(int pin, bool highActive, uint8_t id)
{
    _highActive = highActive;
    {
        _pin = pin;
        pinMode(pin, INPUT);
        _id = id;
        timerData[_id].value = false;
        timerData[_id].autostop = true;
        timerData[_id].pin = pin;
        timerData[_id].countDown = 0;
        timerData[_id].silence = false;
        initTimer();
    }
} 

void PassiveBuzzer::tone(uint32_t frequencyDeciHz, uint32_t durationCentiSec, bool autoStop){
    unsigned long long countdown = 0ULL;

    stopTimer();

    timerData[_id].value = LOW;
    timerData[_id].silence = false;
    timerData[_id].autostop = autoStop;
    countdown = (unsigned long long)durationCentiSec * (unsigned long long)frequencyDeciHz / 5000ULL;
    timerData[_id].countDown = (uint32_t)countdown;

    loadTimer(frequencyDeciHz);
}

void PassiveBuzzer::click(){
    stopTimer();

    timerData[_id].value = HIGH;
    timerData[_id].silence = false;
    timerData[_id].autostop = true;
    timerData[_id].countDown = 5; 

    loadTimer(50000);
}

void PassiveBuzzer::pause(uint32_t durationCentiSec) {
    stopTimer();

    timerData[_id].value = LOW;
    timerData[_id].silence = true;
    timerData[_id].autostop = true;
    timerData[_id].countDown = durationCentiSec;

    loadTimer(5000);
} 

void PassiveBuzzer::stop() {
    stopTimer();
    pinMode(_pin, INPUT);

    timerData[_id].value = LOW;
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

PassiveMelodyBuzzer::PassiveMelodyBuzzer(int8_t pin, bool highActive, uint8_t timerId)
{

    _myBuzzer = PassiveBuzzer::getBuzzer(pin, highActive, timerId);

    _melody = NULL;_nextTone= NULL;
    stop();
}

PassiveMelodyBuzzer::PassiveMelodyBuzzer()
{

    _myBuzzer = NULL;

    _melody = NULL;_nextTone= NULL;
    stop();
}


bool PassiveMelodyBuzzer::begin(int8_t pin, bool highActive, uint8_t timerId)
{

    if (!_myBuzzer)
        _myBuzzer = PassiveBuzzer::getBuzzer(pin, highActive, timerId);
    else
#if defined(ESP32)
    if (pin < 32)
#elif defined(ESP8266)
    if (pin <= 16)
#else 
    if (pin >= 0)
#endif    
        _myBuzzer->pin(pin, highActive);
    stop();
    
    return _myBuzzer != NULL;
}



bool PassiveMelodyBuzzer::busy()
{
bool ret = false;
    if (_myBuzzer) {

        if (_myBuzzer->busy())
        {
            if (_repeatTimeout)
                if (millis() - _repeatStart >= _repeatTimeout)
                    {
                        stop();
                        return false;
                    }
            return true;
        }
        if (_scanPause)
            {
                _myBuzzer->pause(_scanPause);
                _scanPause = 0;
                ret = true;
            }
        else if (_nextTone)
        {
            _nextTone = playTone(_nextTone);
            if (!_nextTone)
            {
                if (_repeat || _repeatTimeout)
                {
                    resetParams();
                    _nextTone = playTone(_melody);
                    if (_repeat)
                        --_repeat;
                    ret = true;
                }
                else
                {
                    stop();
                }
            }
            else
                ret = true;
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
        if (isdigit(*melody))
        {
            _verbose = *melody - '0';
            melody++;
            while ((*melody > 0) && (*melody <= ' '))
                melody++;
        }
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
                if ((*p == ':') || ('s' == tolower(*p)) || ('m' == tolower(*p)))
                {
                    if (isalpha(*p))
                    {
                        _repeatTimeout = _repeat;
                        if ('s' == tolower(*p))
                            _repeatTimeout = _repeatTimeout * 1000;
                        _repeatStart = millis();
                    }
                    p++;
                }
                melody = p;
            }
            _repeat = _repeat - 1;
        }
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



const char * PassiveMelodyBuzzer::playTone(const char *melodyPart)
{
    bool scanSuccess, abort;
    uint32_t toneFreq, toneDuration, scanPause;
    if (!_myBuzzer)
    {
        stop();
        return NULL;
    }
    const char* ret = scanTone(melodyPart, toneFreq, toneDuration, scanPause, scanSuccess, abort);
    _scanPause = scanPause;

    if (!scanSuccess)
    {
        if (abort)
        {
            _repeat = 0;
            _repeatTimeout = 0;
        }
        if ((0 == _repeat) && (0 == _repeatTimeout))
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
    if (_verbose)
        Serial.printf("%4u: Frequency: %4u.%uHz%s, Duration: %5ums (+ %4ums rest, total duration: %5ums)\r\n", 
            ++_debugCount, toneFreq / 10, toneFreq % 10, (0 == toneFreq?"  (Rest)":" (Sound)"), toneDuration,
            scanPause, toneDuration + scanPause);
    if (0 == toneFreq)
        _myBuzzer->pause(toneDuration);
    else
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
        _restChar = _restDefault = _restCharOverride = 0;
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
        resetParams();
        _debugCount = _debugLength = 0;
        _nextTone = NULL;
        _repeatTimeout = 0;
        _repeat = 0;
        _RTTTLd = 0;
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
    while (('*' == *str) || ('/' == *str))
        {
            uint32_t x;
            const char *p = getNumber(str + 1, x);
            if (p)
            {
                if (x > 0)
                {
                    if ('*' == *str)
                        mul = mul * x;
                    else
                        div = div * x;
                }
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

const char* PassiveMelodyBuzzer::scanToneRTTTL(const char* toneString, uint32_t & toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool &scanSuccess, bool &abort)
{
#define RTTTL_SCANSTATE_INIT     0
#define RTTTL_SCANSTATE_DURATION 1    
#define RTTTL_SCANSTATE_NOTE     2
#define RTTTL_SCANSTATE_DELIM    3
#define RTTTL_SCANSTATE_DONE     4
#define RTTTL_SCANSTATE_EMPTY    5
#define RTTTL_SCANSTATE_WAIT     6
#define RTTTL_SCANSTATE_SHARP    7
#define RTTTL_SCANSTATE_DOT      8
#define RTTTL_SCANSTATE_SCALE    9

uint8_t state = RTTTL_SCANSTATE_INIT;
int d = _RTTTLd;
int o;
bool sharp;
bool dot;
char tone = 0;
    abort = false;
//    Serial.printf("Starting Scan: %s\r\n", toneString);
    while ((RTTTL_SCANSTATE_DONE != state) && (RTTTL_SCANSTATE_EMPTY != state))
    {
        char c = tolower(*toneString);
        if ((0 == c) || (',' == c))
        {
            if (0 != c)
                toneString++;
//            if (0 != c)
//                Serial.printf("Delim found, state is: %d, toneString=%s\r\n", state, toneString);
            if (0 != tone)
            {
                state = RTTTL_SCANSTATE_DONE;
            }    
            else
            {
                if (0 == c)
                    state = RTTTL_SCANSTATE_EMPTY;
                else
                    state = RTTTL_SCANSTATE_INIT;
            }
        }
        else if (' ' >= c)
        {
            toneString++;
        }
        else
            switch (state)
            {
                case RTTTL_SCANSTATE_INIT:
                    d = _RTTTLd;
                    o = _octave;
                    sharp = false;
                    state = RTTTL_SCANSTATE_WAIT;
                    dot = false;
                    tone = 0;
                    break;
                case RTTTL_SCANSTATE_WAIT:
//                    Serial.printf("ScanstateWait: '%c' of \"%s\"\r\n", c, toneString);
                    if (((c >= '1') && (c <= '4')) || (c == '8'))
                        state = RTTTL_SCANSTATE_DURATION;
                    else if (((c >= 'a') && (c <= 'h')) || (c == 'p'))
                        state = RTTTL_SCANSTATE_NOTE;
                    else
                        toneString++;
                    break;
                case RTTTL_SCANSTATE_DURATION:
                    d = 0;
                    toneString++;
                    if ((c == '3') && (*toneString == '2'))
                    {
                        d = 32;                        
                    }
                    else if (c == '1')
                    {
                        if (*toneString == '6')
                            d = 16;
                        else
                            d = 1;
                    } 
                    else if ((c == '2') || (c == '4') || (c == '8'))
                    {
                        d = (c - '0');
                    }
//                    Serial.printf("ScanStateDurationDone: %d: '%s'\r\n", d, toneString);
                    if (d > 0)                    
                    {
                        if (d > 10)
                            toneString++;
                        state = RTTTL_SCANSTATE_NOTE;
                    }
                    else
                        state = RTTTL_SCANSTATE_INIT;
                    break;
                case RTTTL_SCANSTATE_NOTE:
                    if (((c >= 'a') && (c <= 'h')) || (c == 'p'))
                    {
                        if ('h' == c)
                            tone = 'b';
                        else
                            tone = c;
 //                       if (NULL != strchr("cdfga", tone))
                        if ('p' != tone)
                            state = RTTTL_SCANSTATE_SHARP;
                        else
                            state = RTTTL_SCANSTATE_SCALE;                    
                    }
                    else
                        state = RTTTL_SCANSTATE_INIT;
//                    Serial.printf("ScanStateNoteDone: tone = '%c', nextState = %d\r\n", tone, state);    
                    toneString++;
                    break;
                case RTTTL_SCANSTATE_SHARP:
                    if ((sharp = ('#' == c)))
                    {
                        toneString++;
                    }
                    state = RTTTL_SCANSTATE_SCALE;
                    break;
                case RTTTL_SCANSTATE_SCALE:
                    if (c == '.')
                    {
                        dot = true;
                        toneString++;
                    }
                    else if ((c >= '4') && (c <= '7'))
                    {
                        o = c - '0';
                        toneString++;
                        state = RTTTL_SCANSTATE_DOT;
                    }
                    else
                        state = RTTTL_SCANSTATE_INIT;
                    break;
                case RTTTL_SCANSTATE_DOT:
                    if ((dot = (c == '.')))
                    {
                        toneString++;
                    }
//                    Serial.printf("State DOT done, entering delim with: %s\r\n", toneString);
                    state = RTTTL_SCANSTATE_DELIM;
                    break;
                default:
                    state = RTTTL_SCANSTATE_INIT;
                    break;
            }
    }

//const char* PassiveMelodyBuzzer::scanToneRTTTL(const char* toneString, uint32_t & toneFreq, uint32_t& toneDuration,
//                        uint32_t& tonePause, bool &scanSuccess, bool &abort)

//    Serial.printf("Scan done, success = %d :'%s'\r\n", RTTTL_SCANSTATE_DONE == state, toneString);
    if ((scanSuccess = (tone != 0)))
    {
        if (tone == 'p')
            toneFreq = 0;
        else
        {
            toneFreq = tones[tone - 'a'].freq[sharp?1:0];
            //if (tone > 'b')
            //    o++;
            o = o - 4;
            //Serial.print("Octave: ");Serial.println(o);
            if (o)
                toneFreq = toneFreq << o;
        }
        toneDuration = _duration / d;
        if (dot)
            toneDuration = toneDuration * 3 / 2;
        return toneString;
    }    
    else
        return NULL;
}

const char* PassiveMelodyBuzzer::scanTone(const char* toneString, uint32_t & toneFreq, uint32_t& toneDuration,
                        uint32_t& tonePause, bool &scanSuccess, bool &abort)
{
    char tone = 0;
    int sign = 0;
    bool prefixDone = false;
    char restChar;
    abort = false;
    tonePause = 0;
    scanSuccess = false;
    if (NULL == toneString)
        return NULL;
    if (_RTTTLd > 0)
        return scanToneRTTTL(toneString, toneFreq, toneDuration, tonePause, scanSuccess, abort);
    while (!prefixDone && (0 != (tone = tolower(*toneString))))
    {
        if (!(prefixDone = ((tone == 'r') || (tone == '@') || ((tone >= 'a') && (tone <= 'g'))))) {
            if ('!' == tone)
            {
                uint32_t x;bool setOverride = false;char restCharx;bool setRelative = false;
                _div = _mul = 1;
                const char *p = NULL;
                if ('=' == toneString[1])
                {
                    toneString++;
                    p = getDuration(toneString + 1, x);
                    _restChar = _restDefault = 0;
                    if (NULL != p)
                    {
                        toneString = p;
                        _duration = x>0?x:60000/DEFAULT_BPM;
                    }
                    else    
                        _duration = 60000/DEFAULT_BPM;
                }
                else if ((setOverride = ('!' == toneString[1])))
                {
                    toneString++;
                    p = getDuration(toneString + 1, x);
                    if (NULL != p)
                    {
                        toneString = p;
                        _div = _mul = 1;
                        _restCharOverride = 0;
                        _durationOverride = x;
                    }
                    else 
                        ;
                }
                else if ((setRelative = (('*' == toneString[1]) || ('/' == toneString[1]))))
                {
                        toneString = getMulDiv(toneString + 1, _mul, _div) - 1;
                }
                else if (('l' == toneString[1]) || ('L' == toneString[1]))
                {
                    uint32_t x;
                    toneString++;
                    _restChar = _restDefault = 0;
                    if (NULL != (p = getLength(toneString, x)))
                    {
                        toneString = p - 1;
                        _duration = x;
                    }
                }
                else                // Special case: ! w/o valid next char
                {
                    setRelative = true;
                }
                if (!setOverride)
                {
                    _restChar = _restDefault;
                    if (('/' == *(toneString + 1)) || ('*' == *(toneString + 1)))
                        toneString = getMulDiv(toneString + 1, _mul, _div) - 1;
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
            else if ((abort = ('x' == tone)))
                return NULL;
        }
        toneString++;
    }
    if (!prefixDone)
    {
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
        if (isalpha(*toneString))
        {
            toneString = getLength(toneString, toneDuration);
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
    if (getRestChar(*toneString, restChar))
    {
        if (0 == toneFreq)
            restChar = 0;
        toneString++;
    }
    if (restChar && (0 != toneFreq))
        {
        restChar = (',' == restChar?_rest1:('.' == restChar?_rest2:(';' == restChar?_rest3:
                    ('\'' == restChar?_rest0:_rest4))));
            tonePause = toneDuration * restChar;
            tonePause = tonePause / 100;
            if (tonePause < toneDuration)
                toneDuration -= tonePause;
            else
                tonePause = 0;
        }
    return toneString;
}

uint32_t PassiveMelodyBuzzer::busyCount() {
    return _myBuzzer?_myBuzzer->busyCount():0;
};

uint8_t PassiveMelodyBuzzer::getId() {
    if (_myBuzzer) 
        return _myBuzzer->getId(); 
    return 255;
};


const char* getRTTTLvaluepair(const char* in, char& control, int& controlvalue)
{
char c = tolower(*in);    
uint32_t x;
control = 0;
    
    if (!isalpha(c))
        return (c != 0)?in + 1:in;
    ++in;
    if (NULL == strchr("odb", c))
        return in;
    while ((*in > 0) && ('=' != *in) && (':' != *in) && (',' != *in))
        in++;
    if (*in != '=')
        return in;
    in++;
    while ((*in > 0) && (':' != *in) && !isdigit(*in))
        in++;
    if (!isdigit(*in))
        return in;
    in = getNumber(in, x) + 1;
    if ('o' == c)
    {
        if ((x < 4) || (x > 7))
            c = 0;
    }
    else if ('b' == c)
    {
        if ((x < 25) || (x > 900))
            c = 0;
    }
    else if ((x != 1) && (x != 2) && (x != 4) && (x != 8) && (x != 16) && (x != 32))
        c = 0;
    if (c != 0)
    {
        control = c;
        controlvalue = x;
    }
    return in;
}

void PassiveMelodyBuzzer::playRTTTL(const char* melody, uint8_t verbose)
{
// https://panuworld.net/nuukiaworld/download/nokix/rtttl.htm
// https://ozekisms.com/p_2375-sms-ringtone-the-rtttl-format.html

    const char* p1;
    int o, b;
    stop();
    p1 = strchr(melody, ':');
    _verbose = verbose;
    if (NULL != p1)
    {
        p1++;
        melody = strchr(p1, ':');
    }
    if ((NULL == p1) || (NULL == melody))
        return;
    melody++;
    _RTTTLd = 4;
    o = 6;
    b = 63;
    while (':' != *p1)
    {
        char control;
        int  controlvalue;
        p1 = getRTTTLvaluepair(p1, control, controlvalue);
        if ('d' == control)
            _RTTTLd = controlvalue;
        else if ('o' == control)
            o = controlvalue;
        else if ('b' == control)
            b = controlvalue;
    }
    _octave = o;
    _duration = 240000 / b;
    _melody = strdup(melody);
    //Serial.printf("o=%d, b=%d, d=%d:%s\r\n", o, b, _RTTTLd, _melody);
    if (NULL == (_nextTone = playTone(_melody)))
        stop();
}
