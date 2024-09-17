/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#include "main.h"
#include "deps/udphandler.h"

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

StaticJsonDocument<256> bufRecv;
StaticJsonDocument<256> bufSend;

//-----------------------------------------------------------------------------
// UDPMANAGE class
//-----------------------------------------------------------------------------
UDPMANAGE::UDPMANAGE(const uint8_t *mac, IPAddress ip, uint8_t deviceID)
{
    char charID[4];
    _selfID = deviceID;
    itoa(deviceID, charID, 10);
    _charID[0] = '\0';
    strcat(_charID, "B");
    strcat(_charID, charID);

    Ethernet.begin(mac, ip);
#ifdef onTEST
    int success = Udp.begin(RECV_PORT);
    Serial.print("initialize: ");
    Serial.println(success ? "success" : "failed");
#else
    Udp.begin(RECV_PORT);
#endif
}

UDPMANAGE::UDPMANAGE() {}

void UDPMANAGE::Init(const uint8_t *mac, IPAddress ip, uint8_t deviceID)
{
    char charID[4];
    _selfID = deviceID;
    itoa(deviceID, charID, 10);
    _charID[0] = '\0';
    strcat(_charID, "B");
    strcat(_charID, charID);

    Ethernet.begin(mac, ip);
#ifdef onTEST
    int success = Udp.begin(RECV_PORT);
    Serial.print("initialize: ");
    Serial.println(success ? "success" : "failed");
#else
    Udp.begin(RECV_PORT);
#endif
}

void UDPMANAGE::UDPsend(IPAddress endNodeIP, uint16_t endPort, const char *command, const char *flag)
{
    int success;
    char temp_message[50];
    temp_message[0] = '\0';
    strcat(temp_message, command);
    strcat(temp_message, _charID);
    strcat(temp_message, flag);
    do
    {
        success = Udp.beginPacket(endNodeIP, endPort);
#ifdef onTEST
        Serial.print("  beginPacket: ");
        Serial.println(success ? "success" : "failed");
#endif
        // beginPacket fails if remote ethaddr is unknown. In this case an
        // arp-request is send out first and beginPacket succeeds as soon
        // the arp-response is received.
    } while (!success);
    if (!success)
        goto stop;

    success = Udp.write(temp_message);
#ifdef onTEST
    Serial.print("  bytes written: ");
    Serial.println(success);
#endif
    success = Udp.endPacket();
#ifdef onTEST
    Serial.print("  endPacket: ");
    Serial.println(success ? "success" : "failed");
#endif
stop:
    Udp.stop(); // restart with new connection to receive packets from other clients
    Serial.print("restart connection: ");
    Serial.println(Udp.begin(RECV_PORT) ? "success" : "failed");
}

void UDPMANAGE::UDPrecv(char *color, uint8_t &port, bool &blink, bool &state, char *from, uint16_t &len)
{
    int size = Udp.parsePacket();
    len = size;
    if (size > 0)
    {
        do
        {
            char *msg = (char *)malloc(size + 1);
            int len = Udp.read(msg, size + 1);
            msg[len] = 0;
#ifdef onTEST
            Serial.print("received: '");
            Serial.print(msg);
            Serial.println("'");
#endif
            DeserializationError error = deserializeJson(bufRecv, msg);
            if (error)
            {
#ifdef onTEST
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
#endif
                return;
            }
            char *sensColor = bufRecv["color"];
            const char *sendFrom = bufRecv["from"];
            memcpy(color, sensColor, LEN_CHARCOLOR);
            memcpy(from, sendFrom, LEN_CHARID);
            port = bufRecv["port"];
            blink = bufRecv["blink"];
            state = bufRecv["state"];
            free(msg);
        } while ((size = Udp.available()) > 0);
        // finish reading this packet:
        Udp.flush();

        Udp.stop(); // restart with new connection to receive packets from other clients
        Serial.print("restart connection: ");
        Serial.println(Udp.begin(RECV_PORT) ? "success" : "failed");
    }
}

void UDPMANAGE::setODLIP(IPAddress &ip)
{
    _odlIP = &ip;
}

void UDPMANAGE::printODLIP()
{
    Serial.println(*_odlIP);
}

void UDPMANAGE::ForwardODL(uint8_t colorCode, uint8_t port, bool isBlink, bool isOn)
{
    char json[100] = {"\0"};
    StaticJsonDocument<200> payload;
    payload["color"] = M_FODL[colorCode];
    payload["port"] = port;
    payload["blink"] = isBlink;
    payload["state"] = isOn;
    payload["from"] = _charID;
    serializeJson(payload, json, 100);

    int success;
    do
    {
        success = Udp.beginPacket(*_odlIP, CM_ODL_PORT);
#ifdef onTEST
        Serial.print("  beginPacket: ");
        Serial.println(success ? "success" : "failed");
#endif
        // beginPacket fails if remote ethaddr is unknown. In this case an
        // arp-request is send out first and beginPacket succeeds as soon
        // the arp-response is received.
    } while (!success);
    if (!success)
        goto stop;

    success = Udp.write(json);
#ifdef onTEST
    Serial.print("  bytes written: ");
    Serial.println(success);
    Serial.print("  written: ");
    Serial.println(json);
#endif
    success = Udp.endPacket();
#ifdef onTEST
    Serial.print("  endPacket: ");
    Serial.println(success ? "success" : "failed");
#endif
stop:
    Udp.stop(); // restart with new connection to receive packets from other clients
    Serial.print("restart connection: ");
    Serial.println(Udp.begin(RECV_PORT) ? "success" : "failed");
}
