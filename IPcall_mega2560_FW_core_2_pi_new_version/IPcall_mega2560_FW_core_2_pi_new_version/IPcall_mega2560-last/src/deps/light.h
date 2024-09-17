/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */
#include "deps/udphandler.h"

typedef struct strQueue
{
    bool isUsed;               // State flag Queue slot
    char user[LEN_CHARID];     // from UDP from
    char color[LEN_CHARCOLOR]; // from UDP color
    bool isblink;              // from UDP blink
    bool isOn;                 // from UDP state
    uint8_t port;              // from UDP port
} Queue;

class LIGHT
{
protected:
    Queue _odlQueue[LEN_QUEUE];
    UDPMANAGE *_cliUDP;
    uint32_t _sRed = 0;
    uint32_t _bRed = 0;
    uint32_t _sGreen = 0;
    uint32_t _bGreen = 0;
    uint32_t _sPendant = 0;
    uint32_t _bPendant = 0;

public:
    LIGHT();
    void Init(UDPMANAGE &UDPcli);
    void loop();
    void getForwardODL();
    void queueFlow(char *color, uint8_t &port, bool &blink, bool &state, char *from);
    uint8_t getEmptySlot();
    void lightControl();
    uint8_t checkInQueue(char *color, char *from);
    uint8_t setToQueue(char *color, uint8_t &port, bool &blink, bool &state, char *from, uint8_t &idx);
    void updateColorBuf(uint8_t colorCode, bool blink, bool state, bool isAdd);
    uint8_t getColorCode(uint8_t idx);
    void _setRed(bool blink, bool state, bool isAdd);
};
