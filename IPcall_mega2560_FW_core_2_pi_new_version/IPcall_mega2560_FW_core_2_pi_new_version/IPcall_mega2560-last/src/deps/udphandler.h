/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#define RECV_PORT 5555      // Receive ODL message Port
#define CM_ODL_PORT 5555    // Commaon ODL Port
#define TEST_IP {192, 168, 1, 254}
// #define loadTEST 1
// #define onTEST true
#ifdef onTEST
#define SERVER_PORT 5555    // Server Port
#else
#define SERVER_PORT 24949    // Server Port
#endif

class UDPMANAGE
{
protected:
    uint8_t _selfID;
    char _charID[LEN_CHARID];
    IPAddress *_odlIP;

public:
    UDPMANAGE(const uint8_t *mac, IPAddress ip, uint8_t deviceID);
    UDPMANAGE();
    void Init(const uint8_t *mac, IPAddress ip, uint8_t deviceID);
    void UDPsend(IPAddress endNodeIP, uint16_t endPort, const char *command, const char *flag);
    void UDPrecv(char *color, uint8_t &port, bool &blink, bool &state, char *from, uint16_t &len);
    void setODLIP(IPAddress &ip);
    void printODLIP();
    void ForwardODL(uint8_t colorCode, uint8_t port, bool isBlink, bool isOn);
};
