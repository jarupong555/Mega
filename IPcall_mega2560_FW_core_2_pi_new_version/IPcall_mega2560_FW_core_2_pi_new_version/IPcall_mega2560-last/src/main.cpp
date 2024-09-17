/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#include "main.h"
#include "deps/IO.h"
#include "deps/light.h"

// for myIP configure
SWx SW7(PORTAPIN, PORTFA);
// for ODLIP configure
SWx SW8(PORTCPIN, PORTFC);
// for serverIP configure (BIT 0-5 MSB(5 4 3 2 1 0))
SWx SW9(PORTLPIN, PORTFL);

LED_control LED_Status(BUILT_IN_LED, COM_CATHODE);
LIGHT lightOB = LIGHT();

LED_control BUZZER_Ob(BUZZER, COM_CATHODE);
LED_control Pcord_led(PULLCORD_LED, COM_ANODE);
LED_control ExCall_led(EX_LED, COM_ANODE);

LED_control LED_call(CALL_LED, COM_ANODE);
LED_control LED_call_cancel(CALL_CANCEL_LED, COM_ANODE);
LED_control LED_pendant(PENDANT_LED, COM_ANODE);
LED_control LED_red(LED_RED, COM_ANODE);
LED_control LED_green(LED_GREEN, COM_ANODE);
LED_control LED_yellow(LED_YELLOW, COM_ANODE);

UDPMANAGE ETHSession;

bool hadPullcord = (SW9.getValue() & BIT0) ? true : false;
bool hadExcall = (SW9.getValue() & BIT1) ? true : false;
#ifndef Serial2Pi
uint8_t deviceID = BitReverseTable256[SW7.getValue()];
uint8_t networkID = (SW9.getValue() & BIT7downto2) >> 2;
IPAddress myIP(192, 168, networkID, deviceID);
uint8_t odlID = SW8.getValue();
IPAddress odlIP(192, 168, networkID, odlID);

#ifdef onTEST
IPAddress serverIP(TEST_IP);
#else
IPAddress serverIP(192, 168, networkID, 100);
#endif
IPAddress gatewayIP(192, 168, networkID, 1);
static byte mac[6] = {0x00, 0x04, 0xA3, 0x03, networkID, deviceID};
#endif

#ifdef loadTEST
volatile uint32_t timeCNT = 0;
volatile uint32_t secCNT = 0;
void testSendToServer()
{
  uint32_t now = millis();
  if (now - timeCNT >= 1000)
  {
    timeCNT = now;

    char charSecCNT[12];
    itoa(secCNT++, charSecCNT, 10);
    char temp_message[50];
    temp_message[0] = '\0';
    strcat(temp_message, "Still alive: ");
    strcat(temp_message, charSecCNT);
    strcat(temp_message, "s. ");

    ETHSession.UDPsend(serverIP, SERVER_PORT, temp_message, " On test load send MSG");
  }
}
#endif

// call 0, pendant 1, pullcord 2, Excall 3, Excall1 4
volatile uint32_t previousMillis[noOfButtons]; // Timers to time out bounce duration for each button
volatile uint8_t pressCount[noOfButtons];      // Counts the number of times the button is detected as pressed, when this count reaches minButtonPress button is regared as debounced

// Recieve Pressed
// BIT0 > callGreen
// BIT1 > callRed
// BIT2 > pendant
// BIT3 > pullcord(PC)
// BIT4 > Excall
// BIT5 > cancelCALL
// BIT6 > cancelPC
// BIT7 > cancelEXCALL
volatile uint8_t allStatus = 0;
// Set when action complete
// BIT0 > done action callGreen
// BIT1 > done action callRed
// BIT2 > done action pendant
// BIT3 > done action pullcord(PC)
// BIT4 > done action Excall
// BIT5 > done action cancelCALL
// BIT6 > done action cancelPC
// BIT7 > done action cancelEXCALL
volatile uint8_t actionStatus = 0;

#ifdef Serial2Pi
String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete
volatile bool stateInCall = false;
volatile uint8_t stateLEDyellowOld = 255;
bool isReady = false;
#else
bool isReady = true;
#endif

void setup()
{
  LED_Status.Blink(ON_TIME, OFF_TIME);

#ifndef Serial2Pi
  ETHSession.Init(mac, myIP, deviceID);
#else
  inputString.reserve(200);
#endif

  Serial.begin(115200);

#ifdef Serial2Pi
  Serial2Pi.begin(115200);
  // while (!Serial2Pi.available())
  // {
  // }
  Serial2Pi.println("preflight");
#endif

#ifndef Serial2Pi
  Serial.println("IPcall service FW V.1.0\n");
  Serial.print("gateway: ");
  Serial.println(gatewayIP);
  Serial.print("Client IP: ");
  Serial.println(myIP);
  Serial.print("Read DIP SW -> ");
  Serial.print("ODL IP: ");
  Serial.println(odlIP);
  Serial.print("Server IP: ");
  Serial.println(serverIP);
  ETHSession.setODLIP(odlIP);
  Serial.print("Used ODL IP -> ");
  ETHSession.printODLIP();

  delay(5000);
  Serial.println("Begin Package pre flight!");
  for (uint8_t cnt = 0; cnt < CNT_PREFLIGHT; cnt++)
  {
    ETHSession.UDPsend(serverIP, SERVER_PORT, "test", "toServer");
    ETHSession.UDPsend(odlIP, CM_ODL_PORT, "test", "toCModl");
    delay(2000);
  }
#endif
  Serial.println("Begin!");

  pinMode(CALL_SW, INPUT_PULLUP);
  pinMode(PENDANT_SW_A0, INPUT_PULLUP);
  pinMode(CALL_CANCEL_SW, INPUT_PULLUP);

  pinMode(PULLCORD_SW, INPUT_PULLUP);
  pinMode(PULLCORD_CANCEL, INPUT_PULLUP);

  pinMode(EX_CALL_SW, INPUT_PULLUP);
  pinMode(EX_CANCEL_SW, INPUT_PULLUP);

  lightOB.Init(ETHSession);

  if (isReady)
  {
    setInterrupt();
  }
}

void loop()
{
  LED_Status.loop();
  lightOB.loop();

  if (isReady)
  {
    call_debounce();
    pendant_debounce();

    pullcord_debounce();

    EXcall_debounce();
    EXcall_debounce1();

    stageManage();
  }
#ifdef Serial2Pi
  else
  {
    checkStart();
  }
#endif

  LED_call.loop();
  LED_yellow.loop();
  LED_call_cancel.loop();
  LED_green.loop();
  LED_pendant.loop();
  LED_red.loop();

  Pcord_led.loop();
  ExCall_led.loop();
  BUZZER_Ob.loop();

#ifdef loadTEST
  testSendToServer();
#endif

#ifdef Serial2Pi
  // print the string when a newline arrives:
  if (stringComplete)
  {
    Serial.println(inputString);
    // clear the string:
    if (inputString.equals("In call"))
    {
      Serial.println("In call state.");
      detachInterrupt(digitalPinToInterrupt(CALL_CANCEL_SW));
      detachInterrupt(digitalPinToInterrupt(PULLCORD_CANCEL));
      // detachInterrupt(digitalPinToInterrupt(EX_CANCEL_SW));

      stateInCall = true;
      stateLEDyellowOld = LED_yellow.getState();
      Serial.println(stateLEDyellowOld);
      LED_yellow.Blink(400, 250);
      BUZZER_Ob.Off();
    }
    else if (inputString.equals("End call"))
    {
      Serial.println("End call.");
      stateInCall = false;
      LED_yellow.setState(stateLEDyellowOld);

      setInterrupt();
    }
    else if (inputString.equals("Ready"))
    {
      setInterrupt();
      isReady = true;
    }
    checkCommonODL(inputString);
    inputString = "";
    stringComplete = false;
  }
  checkResetTimeCounter();
#endif
}

#ifdef Serial2Pi
/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void Serial2PiEvent()
{
  while (Serial2Pi.available())
  {
    // get the new byte:
    char inChar = (char)Serial2Pi.read();
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n')
    {
      stringComplete = true;
      break;
    }
    // add it to the inputString:
    inputString += inChar;
  }
}

void checkStart()
{
  if ((millis() > autoStartAt) && (!isReady))
  {
    BUZZER_Ob.On();
    delay(500);
    BUZZER_Ob.Off();
    delay(500);
    BUZZER_Ob.On();
    delay(200);
    BUZZER_Ob.Off();
    delay(100);
    setInterrupt();
    isReady = true;
  }
}

void checkResetTimeCounter()
{
  if (stateInCall)
  {
    previousMillis[0] = millis();
    previousMillis[1] = millis();
    previousMillis[2] = millis();
    previousMillis[3] = millis();
  }
}

void checkCommonODL(String data)
{
  uint8_t port = 255;
  char from[LEN_CHARID] = "cmodl";
  char state = data.charAt(0);
  char blink = data.charAt(1);

#ifdef onTEST
  Serial.println(data);
  Serial.print("GYR: ");
  Serial.print(data.endsWith("green"));
  Serial.print(data.endsWith("yellow"));
  Serial.println(data.endsWith("red"));
  Serial.print("state, blink: ");
  Serial.print(data.charAt(0));
  Serial.println(data.charAt(1));
#endif

  if (data.endsWith("green"))
  {
    char color[LEN_CHARCOLOR] = "green";
    bool Bblink = false;
    bool Bstate = true;
    if (state == '1')
    {
      if (blink == '1')
      {
        Bblink = true;
        lightOB.queueFlow(color, port, Bblink, Bstate, from);
      }
      else
      {
        lightOB.queueFlow(color, port, Bblink, Bstate, from);
      }
    }
    else
    {
      Bstate = false;
      lightOB.queueFlow(color, port, Bblink, Bstate, from);
    }
  }
  else if (data.endsWith("yellow"))
  {
    char color[LEN_CHARCOLOR] = "pendant";
    bool Bblink = false;
    bool Bstate = true;
    if (state == '1')
    {
      if (blink == '1')
      {
        Bblink = true;
        lightOB.queueFlow(color, port, Bblink, Bstate, from);
      }
      else
      {
        lightOB.queueFlow(color, port, Bblink, Bstate, from);
      }
    }
    else
    {
      Bstate = false;
      lightOB.queueFlow(color, port, Bblink, Bstate, from);
    }
  }
  else if (data.endsWith("red"))
  {
    char color[LEN_CHARCOLOR] = "red";
    bool Bblink = false;
    bool Bstate = true;
    if (state == '1')
    {
      if (blink == '1')
      {
        Bblink = true;
        lightOB.queueFlow(color, port, Bblink, Bstate, from);
      }
      else
      {
        lightOB.queueFlow(color, port, Bblink, Bstate, from);
      }
    }
    else
    {
      Bstate = false;
      lightOB.queueFlow(color, port, Bblink, Bstate, from);
    }
  }
}
#endif

void setInterrupt()
{
  attachInterrupt(digitalPinToInterrupt(CALL_CANCEL_SW), callCancelSWHandler, FALLING);
  if (hadPullcord)
  {
    attachInterrupt(digitalPinToInterrupt(PULLCORD_CANCEL), pullcordCancelSWHandler, FALLING);
  }
  // if (hadExcall)
  // {
  //   attachInterrupt(digitalPinToInterrupt(EX_CANCEL_SW), EXcallCancelSWHandler, FALLING);
  // }
  LED_call.On();
}

void stageManage()
{
  uint8_t port = 255;
  char from[LEN_CHARID] = "self";

  // 0
  // call Green
  if ((allStatus & BIT0) && ((actionStatus & BIT0) == 0))
  {
    Serial.println("Green action");
    LED_green.On();
    LED_call_cancel.On();

#ifdef Serial2Pi
    Serial2Pi.println("callGreen");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_SETACTIVE, M_CALLGREEN);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "green";
      bool blink = false;
      bool state = true;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(GREEN, PORT_CODE_ODL_G, false, true);
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT0;
  }

  // 1
  // call Red
  if ((allStatus & BIT1) && ((actionStatus & BIT1) == 0))
  {
    Serial.println("Red action");
    LED_red.toggle(1);
    LED_call_cancel.On();
    BUZZER_Ob.toggle(1);

#ifdef Serial2Pi
    Serial2Pi.println("callRed");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_SETACTIVE, M_CALLEMER);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "red";
      bool blink = true;
      bool state = true;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(RED, PORT_CODE_ODL_R, true, true);
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT1;
  }

  // 2
  // Pendant
  if ((allStatus & BIT2) && ((actionStatus & BIT2) == 0))
  {
    Serial.println("Pendant action");
    LED_pendant.Blink(500, 300);
    LED_yellow.On();
    LED_call_cancel.On();

#ifdef Serial2Pi
    Serial2Pi.println("pendant");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_SETACTIVE, M_PC_CALL);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "pendant";
      bool blink = false;
      bool state = true;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(PENDANT, PORT_CODE_ODL_Y, false, true);
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT2;
  }

  // 3
  // Pullcord
  if ((allStatus & BIT3) && ((actionStatus & BIT3) == 0))
  {
    Serial.println("Pullcord action");
    Pcord_led.On();
    // LED_red.toggle(2);

#ifdef Serial2Pi
    Serial2Pi.println("pullcord");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_SETACTIVE, M_PC_CALL);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "pullcord";
      bool blink = true;
      bool state = true;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(PCORD, 254, false, true);
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT3;
  }

  // 4
  // Ex-Call
  if ((allStatus & BIT4) && ((actionStatus & BIT4) == 0))
  {
    Serial.println("ExCall action");
    LED_yellow.toggle(1);
    ExCall_led.On();
    // LED_red.toggle(2);
    LED_call_cancel.On();

#ifdef Serial2Pi
    Serial2Pi.println("exCall");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_SETACTIVE, M_PC_CALL);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "emer";
      bool blink = true;
      bool state = true;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(EMER, 254, true, true);
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT4;
  }

  // 5
  // cancel CALL SW
  if (allStatus & BIT5 && ((actionStatus & BIT5) == 0))
  {
    Serial.println("Call Cancel action");
    // Green call
    if (allStatus & BIT0)
    {
      // Clear all flag
      allStatus = allStatus & (uint8_t(~BIT0));
      LED_green.Off();

#ifdef Serial2Pi
      Serial2Pi.println("canGreen");
      if (true)
#else
      ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_CALLGREEN);
      if (odlID == 0)
#endif
      {
        char color[LEN_CHARCOLOR] = "green";
        char from[LEN_CHARID] = "self";
        // uint8_t port = 255;
        bool blink = false;
        bool state = false;
        lightOB.queueFlow(color, port, blink, state, from);
      }
      else
      {
        ETHSession.ForwardODL(GREEN, PORT_CODE_ODL_G, false, false);
      }
    }
    // Red call
    if (allStatus & BIT1)
    {
      // Clear all flag
      allStatus = allStatus & (uint8_t(~BIT1));
      LED_red.Off();

#ifdef Serial2Pi
      Serial2Pi.println("canRed");
      if (true)
#else
      ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_CALLEMER);
      if (odlID == 0)
#endif
      {
        char color[LEN_CHARCOLOR] = "red";
        char from[LEN_CHARID] = "self";
        // uint8_t port = 255;
        bool blink = true;
        bool state = false;
        lightOB.queueFlow(color, port, blink, state, from);
      }
      else
      {
        ETHSession.ForwardODL(RED, PORT_CODE_ODL_R, true, false);
        // ODL_red.Off();
      }
    }
    // must edit
    // Pendant call
    if (digitalRead(PENDANT_SW_A0))
    {
      LED_pendant.Off();
      LED_yellow.Off();

      if (allStatus & BIT2)
      {
        allStatus = allStatus & (uint8_t(~BIT2));
#ifdef Serial2Pi
        Serial2Pi.println("canPendant");
        if (true)
#else
        ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_PC_CALL);
        if (odlID == 0)
#endif
        {
          char color[LEN_CHARCOLOR] = "pendant";
          char from[LEN_CHARID] = "self";
          // uint8_t port = 255;
          bool blink = false;
          bool state = false;
          lightOB.queueFlow(color, port, blink, state, from);
        }
        else
        {
          ETHSession.ForwardODL(PENDANT, PORT_CODE_ODL_Y, false, false);
          // ODL_yellow.Off();
        }
      }
    }
    // ex call
    if (allStatus & BIT4)
    {
      // Clear all flag
      allStatus = allStatus & (uint8_t(~BIT4));
      ExCall_led.Off();

#ifdef Serial2Pi
      Serial2Pi.println("canExcall");
      if (true)
#else
      ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_CALLGREEN);
      if (odlID == 0)
#endif
      {
        char color[LEN_CHARCOLOR] = "emer";
        char from[LEN_CHARID] = "self";
        // uint8_t port = 255;
        bool blink = true;
        bool state = false;
        lightOB.queueFlow(color, port, blink, state, from);
      }
      else
      {
        ETHSession.ForwardODL(EMER, 254, true, false);
      }
    }
    // ex call1
    if (allStatus & BIT7)
    {
      // Clear all flag
      allStatus = allStatus & (uint8_t(~BIT7));
      ExCall_led.Off();

#ifdef Serial2Pi
      Serial2Pi.println("canExcall1");
      if (true)
#else
      ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_CALLGREEN);
      if (odlID == 0)
#endif
      {
        char color[LEN_CHARCOLOR] = "emer1";
        char from[LEN_CHARID] = "self";
        // uint8_t port = 255;
        bool blink = true;
        bool state = false;
        lightOB.queueFlow(color, port, blink, state, from);
      }
      else
      {
        ETHSession.ForwardODL(EMER1, 254, true, false);
      }
    }

    if ((allStatus & (BIT2downto0 | BIT4 | BIT7)) == 0)
    {
      LED_call_cancel.Off();
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT5;
  }

  // 6
  // cancel Pullcord SW
  if (allStatus & BIT6 && ((actionStatus & BIT6) == 0))
  {
    Serial.println("PC cancel action");
    Pcord_led.Off();
    LED_red.Off();

#ifdef Serial2Pi
    Serial2Pi.println("canPullcord");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_PC_CALL);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "pullcord";
      bool blink = false;
      bool state = false;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(PCORD, 254, false, false);
    }
    // Clear all flag
    allStatus = allStatus & (uint8_t(~BIT3));

    // Set flag action complete
    actionStatus = actionStatus | BIT6;
  }

  // 7
  // EXcall 1
  if (allStatus & BIT7 && ((actionStatus & BIT7) == 0))
  {
    Serial.println("ExCall1 action");
    ExCall_led.On();
    LED_yellow.toggle(1);
    // LED_red.toggle(3);
    LED_call_cancel.On();

#ifdef Serial2Pi
    Serial2Pi.println("exCall1");
    if (true)
#else
    ETHSession.UDPsend(serverIP, SERVER_PORT, M_CALL_DEACTIVE, M_PC_CALL);
    if (odlID == 0)
#endif
    {
      char color[LEN_CHARCOLOR] = "emer1";
      bool blink = true;
      bool state = true;
      lightOB.queueFlow(color, port, blink, state, from);
    }
    else
    {
      ETHSession.ForwardODL(EMER1, 254, true, true);
    }

    // Set flag action complete
    actionStatus = actionStatus | BIT7;
  }

  return;
}

void callCancelSWHandler()
{
  if ((allStatus & (BIT2downto0 | BIT4 | BIT7)) == 0)
  {
    return;
  }
  detachInterrupt(digitalPinToInterrupt(CALL_CANCEL_SW));

  uint8_t i = 0;
#ifdef onTEST
  Serial.println("call cancel pressed");
#endif
  while (!digitalRead(CALL_CANCEL_SW))
  {
    // check call green, red, pendant pressed
    if ((allStatus & (BIT2downto0 | BIT4 | BIT7)) == 0)
    {
      break;
    }
    i++;
    if (i == 0)
    {
      allStatus = allStatus | BIT5;                   // call cancel flag
      actionStatus = actionStatus & (uint8_t(~BIT5)); // set to do flag
      BUZZER_Ob.On(TIME_BUZZER_BEEP);
      break;
    }
  }
  attachInterrupt(digitalPinToInterrupt(CALL_CANCEL_SW), callCancelSWHandler, FALLING);
}

void pullcordCancelSWHandler()
{
  if ((allStatus & BIT3) == 0)
  {
    return;
  }
  detachInterrupt(digitalPinToInterrupt(PULLCORD_CANCEL));

  uint16_t i = 0;
#ifdef onTEST
  Serial.println("pullcord cancel pressed");
#endif
  while (!digitalRead(PULLCORD_CANCEL))
  {
    // check pullcord pressed
    if ((allStatus & BIT3) == 0)
    {
      break;
    }
    i++;
    if (i == 0)
    {
      allStatus = allStatus | BIT6;                   // PC cancel flag
      actionStatus = actionStatus & (uint8_t(~BIT6)); // set to do flag
      // BUZZER_Ob.On(TIME_BUZZER_BEEP);
      break;
    }
  }
  attachInterrupt(digitalPinToInterrupt(PULLCORD_CANCEL), pullcordCancelSWHandler, FALLING);
}

// void EXcallCancelSWHandler()
// {
//   if ((allStatus & BIT4) == 0)
//   {
//     return;
//   }
//   detachInterrupt(digitalPinToInterrupt(EX_CANCEL_SW));

//   uint8_t i = 0;
// #ifdef onTEST
//   Serial.println("Ex call cancel pressed");
// #endif
//   while (!digitalRead(EX_CANCEL_SW))
//   {
//     // check Ex call pressed
//     if ((allStatus & BIT4) == 0)
//     {
//       break;
//     }
//     i++;
//     if (i == 0)
//     {
//       allStatus = allStatus | BIT7;                   // Ex call cancel flag
//       actionStatus = actionStatus & (uint8_t(~BIT7)); // set to do flag
//       BUZZER_Ob.On(TIME_BUZZER_BEEP);
//       break;
//     }
//   }
//   attachInterrupt(digitalPinToInterrupt(EX_CANCEL_SW), EXcallCancelSWHandler, FALLING);
// }

void call_debounce()
{
  if ((allStatus & BIT1downto0) == BIT1downto0)
  {
    return;
  }
  uint32_t currentMillis = millis();
  if (digitalRead(CALL_SW)) // release botton
  {
    if ((minButtonPress < pressCount[0]) & (pressCount[0] < maxButtonPress))
    {
      if ((allStatus & BIT0) == BIT0)
      {
        previousMillis[0] = currentMillis;
        pressCount[0] = 0;
        return;
      }
#ifdef onTEST
      Serial.println("call pressed");
      Serial.println(allStatus);
      Serial.println(actionStatus);
#endif
      allStatus = allStatus | BIT0;                   // call Green flag
      actionStatus = actionStatus & (uint8_t(~BIT0)); // set to do flag
#ifdef onTEST
      Serial.println(actionStatus);
      Serial.println("after");
      Serial.println(allStatus);
      Serial.println(actionStatus);
#endif
    }
    // Input is high, button not pressed or in the middle of bouncing and happens to be high
    previousMillis[0] = currentMillis; // Set previousMillis to millis to reset timeout
    pressCount[0] = 0;                 // Set the number of times the button has been detected as pressed to 0
  }
  else
  {
    // BEEP for green state
    if (currentMillis - previousMillis[0] > bounceDelay)
    {
      previousMillis[0] = currentMillis; // Set previousMillis to millis to reset timeout
      ++pressCount[0];
      if (pressCount[0] == minButtonPress)
      {
        bool isToggle = false;
        if ((BUZZER_Ob.getState() & BIT6downto0) == TOGGLE)
        {
          isToggle = true;
        }
        BUZZER_Ob.On(TIME_BUZZER_BEEP);
        if (isToggle)
        {
          BUZZER_Ob.toggle(1);
        }
      }
    }

    // red state condition
    if (LED_red.getState() != TOGGLE)
    {
      if (pressCount[0] == maxButtonPress)
      {
        BUZZER_Ob.On(255);
#ifdef onTEST
        Serial.println("Emer red pressed");
#endif
        allStatus = allStatus | BIT1;                   // call Red flag
        actionStatus = actionStatus & (uint8_t(~BIT1)); // set to do flag
      }
    }
  }
}

void pendant_debounce()
{
  if ((allStatus & BIT2) == BIT2)
  {
    return;
  }

  uint32_t currentMillis = millis();
  if (digitalRead(PENDANT_SW_A0))
  {                                    // Input is high, button not pressed or in the middle of bouncing and happens to be high
    previousMillis[1] = currentMillis; // Set previousMillis to millis to reset timeout
    pressCount[1] = 0;                 // Set the number of times the button has been detected as pressed to 0
  }
  else
  {
    if (currentMillis - previousMillis[1] > bounceDelay)
    {
      previousMillis[1] = currentMillis; // Set previousMillis to millis to reset timeout
      ++pressCount[1];
      if (LED_yellow.getState() != ON)
      {
        if (pressCount[1] == minButtonPress)
        {
#ifdef onTEST
          Serial.println("pendant pressed");
#endif
          allStatus = allStatus | BIT2;                   // call Pendant flag
          actionStatus = actionStatus & (uint8_t(~BIT2)); // set to do flag
          BUZZER_Ob.On(TIME_BUZZER_BEEP + 50);
        }
      }
    }
  }
}

void pullcord_debounce()
{
  if ((allStatus & BIT3) == BIT3 || (hadPullcord == 0))
  {
    return;
  }

  uint32_t currentMillis = millis();
  if (!digitalRead(PULLCORD_SW))
  {                                    // Input is high, button not pressed or in the middle of bouncing and happens to be high
    previousMillis[2] = currentMillis; // Set previousMillis to millis to reset timeout
    pressCount[2] = 0;                 // Set the number of times the button has been detected as pressed to 0
  }
  else
  {
    if (currentMillis - previousMillis[2] > bounceDelay)
    {
      previousMillis[2] = currentMillis; // Set previousMillis to millis to reset timeout
      ++pressCount[2];
      if (Pcord_led.getState() != ON)
      {
        if (pressCount[2] == minButtonPress)
        {
#ifdef onTEST
          Serial.println("pullcord pressed");
#endif
          allStatus = allStatus | BIT3;                   // call Pull Cord flag
          actionStatus = actionStatus & (uint8_t(~BIT3)); // set to do flag
          // BUZZER_Ob.On(TIME_BUZZER_BEEP + 50);
        }
      }
    }
  }
}

void EXcall_debounce()
{
  if (((allStatus & BIT4) == BIT4) || (hadExcall == 0))
  {
    return;
  }

  uint32_t currentMillis = millis();
  if (digitalRead(EX_CALL_SW))
  {                                    // Input is high, button not pressed or in the middle of bouncing and happens to be high
    previousMillis[3] = currentMillis; // Set previousMillis to millis to reset timeout
    pressCount[3] = 0;                 // Set the number of times the button has been detected as pressed to 0
  }
  else
  {
    if (currentMillis - previousMillis[3] > bounceDelay)
    {
      previousMillis[3] = currentMillis; // Set previousMillis to millis to reset timeout
      ++pressCount[3];
     // if (ExCall_led.getState() != ON)
     // {
        if (pressCount[3] == minButtonPress)
        {
#ifdef onTEST
          Serial.println("Ex Call pressed");
#endif
          allStatus = allStatus | BIT4;                   // call Pull Cord flag
          actionStatus = actionStatus & (uint8_t(~BIT4)); // set to do flag
          BUZZER_Ob.On(TIME_BUZZER_BEEP + 50);
        }
     // }
    }
  }
}

void EXcall_debounce1()
{
  if (((allStatus & BIT7) == BIT7) || (hadExcall == 0))
  {
    return;
  }

  uint32_t currentMillis = millis();
  if (digitalRead(EX_CANCEL_SW))
  {                                    // Input is high, button not pressed or in the middle of bouncing and happens to be high
    previousMillis[4] = currentMillis; // Set previousMillis to millis to reset timeout
    pressCount[4] = 0;                 // Set the number of times the button has been detected as pressed to 0
  }
  else
  {
    if (currentMillis - previousMillis[4] > bounceDelay)
    {
      previousMillis[4] = currentMillis; // Set previousMillis to millis to reset timeout
      ++pressCount[4];
     // if (ExCall_led.getState() != ON)
     //   {
        if (pressCount[4] == minButtonPress)
        {
#ifdef onTEST
          Serial.println("Ex1 Call pressed");
#endif
          allStatus = allStatus | BIT7;                   // call Pull Cord flag
          actionStatus = actionStatus & (uint8_t(~BIT7)); // set to do flag
          BUZZER_Ob.On(TIME_BUZZER_BEEP + 50);
        }
     // }
    }
  }
}
