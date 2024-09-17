/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#include "main.h"
#include "deps/light.h"
#include "deps/IO.h"

LED_control ODL_yellow(ODL_YELLOW, COM_ANODE);
LED_control ODL_green(ODL_GREEN, COM_ANODE);
LED_control ODL_red(ODL_RED, COM_ANODE);

//-----------------------------------------------------------------------------
// LIGHT class
//-----------------------------------------------------------------------------
LIGHT::LIGHT() {}

void LIGHT::Init(UDPMANAGE &UDPcli)
{
    _cliUDP = &UDPcli;
}

void LIGHT::loop()
{
    getForwardODL();

    ODL_green.loop();
    ODL_yellow.loop();
    ODL_red.loop();
}

void LIGHT::getForwardODL()
{
    char color[LEN_CHARCOLOR] = {'\0'};
    uint8_t port = 255;
    bool blink = false;
    bool state = false;
    char from[LEN_CHARID] = {'\0'};
    uint16_t size = 0;
    _cliUDP->UDPrecv(color, port, blink, state, from, size);

    if (size > 0)
    {
        queueFlow(color, port, blink, state, from);
    }
}

void LIGHT::queueFlow(char *color, uint8_t &port, bool &blink, bool &state, char *from)
{
    uint8_t idInQueue = checkInQueue(color, from);
#ifdef onTEST
    Serial.print("\tidInQueue(254 <> None):");
    Serial.println(idInQueue);
#endif
    if (idInQueue == NONE) // New item not in Queue
    {
        uint8_t id = getEmptySlot();
#ifdef onTEST
        Serial.print("\tEmpty slot in Queue: ");
        Serial.println(id);
#endif
        if (id != FULL)
        {
            uint8_t colorCode = setToQueue(color, port, blink, state, from, id);
            updateColorBuf(colorCode, blink, state, true);
        }
    }
    else
    {
        if (state == false)
        {
            _odlQueue[idInQueue].isUsed = false;
            uint8_t colorCode = getColorCode(idInQueue);
            updateColorBuf(colorCode, blink, state, false);
        }
    }
#ifdef onTEST
    Serial.print("From host: ");
    Serial.println(from);

    Serial.print("state red: ");
    Serial.println(_sRed);
    Serial.print("blink red: ");
    Serial.println(_bRed);
    Serial.print("state green: ");
    Serial.println(_sGreen);
    Serial.print("blink green: ");
    Serial.println(_bGreen);
    Serial.print("state pendant: ");
    Serial.println(_sPendant);
    Serial.print("blink pendant: ");
    Serial.println(_bPendant);
#endif

    lightControl();
}

uint8_t LIGHT::getEmptySlot()
{
    for (uint8_t idx = 0; idx < LEN_QUEUE; idx++)
    {
        if (_odlQueue[idx].isUsed != true)
        {
            return idx;
        }
    }
    return FULL;
}

void LIGHT::lightControl()
{
    uint8_t isRedToggle = ODL_red.getState() & BIT6downto0;
    if (_sRed > 0)
    {
        if (_bRed > 0)
        {
            if (isRedToggle != TOGGLE)
            {
                ODL_red.toggle(TOGGLE_TIME);
            }
        }
        else
        {
            ODL_red.On();
        }
    }
    else
    {
        ODL_red.Off();
    }

    uint8_t isGreenToggle = ODL_green.getState() & BIT6downto0;
    if (_sGreen > 0)
    {
        if (_bGreen > 0)
        {
            if (isGreenToggle != TOGGLE)
            {
                ODL_green.toggle(TOGGLE_TIME);
            }
        }
        else
        {
            ODL_green.On();
        }
    }
    else
    {
        ODL_green.Off();
    }

    uint8_t isPendantToggle = ODL_yellow.getState() & BIT6downto0;
    if (_sPendant > 0)
    {
        if (_bPendant > 0)
        {
            if (isPendantToggle != TOGGLE)
            {
                ODL_yellow.toggle(TOGGLE_TIME);
            }
        }
        else
        {
            ODL_yellow.On();
        }
    }
    else
    {
        ODL_yellow.Off();
    }
}

uint8_t LIGHT::checkInQueue(char *color, char *from)
{
    for (uint8_t idx = 0; idx < LEN_QUEUE; idx++)
    {
        if (_odlQueue[idx].isUsed == true)
        {
            int isSameUser = strcmp(from, _odlQueue[idx].user);
            int isSameColor = strcmp(color, _odlQueue[idx].color);
            if ((isSameColor == 0) && (isSameUser == 0))
            {
                return idx;
            }
        }
    }
    return NONE;
}

uint8_t LIGHT::setToQueue(char *color, uint8_t &port, bool &blink, bool &state, char *from, uint8_t &idx)
{
    memcpy(_odlQueue[idx].color, color, LEN_CHARCOLOR);
    _odlQueue[idx].port = port;
    _odlQueue[idx].isblink = blink;
    _odlQueue[idx].isOn = state;
    memcpy(_odlQueue[idx].user, from, LEN_CHARID);
    _odlQueue[idx].isUsed = true;

    return getColorCode(idx);
}

void LIGHT::updateColorBuf(uint8_t colorCode, bool blink, bool state, bool isAdd)
{
    switch (colorCode)
    {
    case RED:
        _setRed(blink, state, isAdd);
        break;
    case GREEN:
        if (isAdd)
        {
            if (blink)
            {
                _bGreen = _bGreen << 1;
                _bGreen = _bGreen | 0x01;
            }
            if (state)
            {
                _sGreen = _sGreen << 1;
                _sGreen = _sGreen | 0x01;
            }
        }
        else
        {
            _sGreen = _sGreen >> 1;
            if (blink)
            {
                _bGreen = _bGreen >> 1;
            }
        }
        break;
    case PENDANT:
        if (isAdd)
        {
            if (blink)
            {
                _bPendant = _bPendant << 1;
                _bPendant = _bPendant | 0x01;
            }
            if (state)
            {
                _sPendant = _sPendant << 1;
                _sPendant = _sPendant | 0x01;
            }
        }
        else
        {
            _sPendant = _sPendant >> 1;
            if (blink)
            {
                _bPendant = _bPendant >> 1;
            }
        }
        break;
    case EMER:
        if (isAdd)
        {
            if (blink)
            {
                _bPendant = _bPendant << 1;
                _bPendant = _bPendant | 0x01;
            }
            if (state)
            {
                _sPendant = _sPendant << 1;
                _sPendant = _sPendant | 0x01;
            }
        }
        else
        {
            _sPendant = _sPendant >> 1;
            if (blink)
            {
                _bPendant = _bPendant >> 1;
            }
        }
        break;
    case EMER1:
        if (isAdd)
        {
            if (blink)
            {
                _bPendant = _bPendant << 1;
                _bPendant = _bPendant | 0x01;
            }
            if (state)
            {
                _sPendant = _sPendant << 1;
                _sPendant = _sPendant | 0x01;
            }
        }
        else
        {
            _sPendant = _sPendant >> 1;
            if (blink)
            {
                _bPendant = _bPendant >> 1;
            }
        }
        break;
    case PCORD:
        _setRed(blink, state, isAdd);
        break;
    default:
        break;
    }
}

uint8_t LIGHT::getColorCode(uint8_t idx)
{
    if (strcmp(_odlQueue[idx].color, "red") == 0)
    {
        return RED;
    }
    if (strcmp(_odlQueue[idx].color, "green") == 0)
    {
        return GREEN;
    }
    if (strcmp(_odlQueue[idx].color, "pendant") == 0)
    {
        return PENDANT;
    }
    if (strcmp(_odlQueue[idx].color, "emer") == 0)
    {
        return EMER;
    }
    if (strcmp(_odlQueue[idx].color, "emer1") == 0)
    {
        return EMER1;
    }
    if (strcmp(_odlQueue[idx].color, "pullcord") == 0)
    {
        return PCORD;
    }
    return NONE;
}

void LIGHT::_setRed(bool blink, bool state, bool isAdd)
{
    if (isAdd)
    {
        if (blink)
        {
            _bRed = _bRed << 1;
            _bRed = _bRed | 0x01;
        }
        if (state)
        {
            _sRed = _sRed << 1;
            _sRed = _sRed | 0x01;
        }
    }
    else
    {
        _sRed = _sRed >> 1;
        if (blink)
        {
            _bRed = _bRed >> 1;
        }
    }
}
