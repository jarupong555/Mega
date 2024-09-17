/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#include "IO.h"
#include "deps/constant.h"

//-----------------------------------------------------------------------------
// LED class
//-----------------------------------------------------------------------------
LED_control::LED_control(uint8_t pin, boolean comType)
{
    _ledPin = pin;
    _pinType = comType;
    _timeStart = millis();
    _keepTime = _timeStart;
    setState(OFF);
    pinMode(pin, OUTPUT); // set LED  pin as OUTPUT
    Off();
}

LED_control::LED_control(void) {}

void LED_control::On()
{
    setState(ON);
    if (_pinType)
    {
        digitalWrite(_ledPin, LOW);
    }
    else
    {
        digitalWrite(_ledPin, HIGH);
    }
}

void LED_control::On(uint8_t setTime)
{
    _keepTime = (uint32_t)setTime;
    setState(ON | BIT7);
    if (_pinType)
    {
        digitalWrite(_ledPin, LOW);
    }
    else
    {
        digitalWrite(_ledPin, HIGH);
    }
    setTimeStart(millis());
}

void LED_control::Init(uint8_t pin, boolean comType)
{
    _ledPin = pin;
    _pinType = comType;
    _timeStart = millis();
    _keepTime = _timeStart;
    setState(OFF);
    pinMode(pin, OUTPUT); // set LED  pin as OUTPUT
    Off();
}

void LED_control::Off()
{
    setState(OFF);
    if (_pinType)
    {
        digitalWrite(_ledPin, HIGH);
    }
    else
    {
        digitalWrite(_ledPin, LOW);
    }
}

void LED_control::Blink(uint16_t on, uint16_t off)
{
    _onTime = on;
    _offTime = off;
    setState(BLINK);
    setTimeStart(millis());
}

void LED_control::loop()
{
    uint32_t current = millis();
    uint8_t temp = getState();
    switch (temp & BIT6downto0) // focus on bit 6-0
    {
    case OFF:
        break;
    case TOGGLE:
        if (current - _timeStart > _keepTime)
        {
            if (temp & BIT7)
            {
                On();
                setState(temp & BIT6downto0);
            }
            else
            {
                Off();
                setState(temp | BIT7);
            }
            setTimeStart(millis());
        }
        break;
    case ON:
        if (temp & BIT7)
        {
            if (current - _timeStart > _keepTime)
            {
                Off();
            }
        }
        break;
    case BLINK:
        if (current - _timeStart > _keepTime)
        {
            if (temp & BIT7)
            {
                _keepTime = (uint32_t)_offTime;
                setState(temp & BIT6downto0);
            }
            else
            {
                _keepTime = (uint32_t)_onTime;
                setState(temp | BIT7);
            }
            digitalWrite(_ledPin, !getPinState());
            setTimeStart(millis());
        }
        break;
    default:
        Off();
        break;
    }
}

void LED_control::setState(uint8_t setTo)
{
    _state = setTo;
}

void LED_control::toggle(uint8_t sec)
{
    setState(TOGGLE);
    setKeepTime(uint32_t(sec) * uint32_t(1000));
    setTimeStart(millis());
    digitalWrite(_ledPin, !getPinState());
}

void LED_control::setKeepTime(uint32_t millisec)
{
    _keepTime = millisec;
}
void LED_control::setTimeStart(uint32_t millisec)
{
    _timeStart = millisec;
}

bool LED_control::getPinState()
{
    return digitalRead(_ledPin);
}

uint8_t LED_control::getState()
{
    return _state;
}

uint8_t LED_control::getPin()
{
    return _ledPin;
}

//-----------------------------------------------------------------------------
// SWx SELECT class
//-----------------------------------------------------------------------------
SWx::SWx(const uint8_t *portfamily, uint8_t group)
{
    _portGroup = group;
    for (uint8_t cnt; cnt < 8; cnt++)
    {
        _portPin[cnt] = portfamily[cnt];
        pinMode(portfamily[cnt], INPUT_PULLUP); // set IPSELECT  pin as input_pullup
    }
}

SWx::SWx() {}

void SWx::Init(const uint8_t *portfamily, uint8_t group)
{
    _portGroup = group;
    for (uint8_t cnt; cnt < 8; cnt++)
    {
        _portPin[cnt] = portfamily[cnt];
        pinMode(portfamily[cnt], INPUT_PULLUP); // set IPSELECT  pin as input_pullup
    }
}

uint8_t SWx::getValue()
{
    uint8_t val = 0;
    switch (_portGroup)
    {
    case PORTFA:
        val = PINA;
        break;
    case PORTFB:
        val = PINB;
        break;
    case PORTFC:
        val = PINC;
        break;
    case PORTFD:
        val = PIND;
        break;
    case PORTFE:
        val = PINE;
        break;
    case PORTFF:
        val = PINF;
        break;
    case PORTFG:
        val = PING;
        break;
    case PORTFH:
        val = PINH;
        break;
    case PORTFJ:
        val = PINJ;
        break;
    case PORTFK:
        val = PINK;
        break;
    case PORTFL:
        val = PINL;
        break;
    default:
        val = 255;
        break;
    }
    return val ^ 0xFF;
}
