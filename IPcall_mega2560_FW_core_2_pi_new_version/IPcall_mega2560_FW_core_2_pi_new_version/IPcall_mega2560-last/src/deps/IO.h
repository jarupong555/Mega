/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#include <arduino.h>

//-------------------------------------------------------
/*
 * For Common Cathode LED configure
 * Common Cathode -> Cathode terminal connected to GND
 * Anode connected to I/O used to control it
 *
 */
class LED_control
{
protected:
  uint8_t _state;
  uint8_t _ledPin;
  uint8_t _pinType;
  uint16_t _onTime;
  uint16_t _offTime;
  uint32_t _keepTime;
  uint32_t _timeStart;

public:
  LED_control(uint8_t pin, boolean comType);
  LED_control(void);
  void Init(uint8_t pin, boolean comType);
  void On();                // ON forever
  void On(uint8_t setTime); // ON time x ms(8 bits)
  void Off();
  void Blink(uint16_t on, uint16_t off);
  void loop();
  void setState(uint8_t setTo);
  void toggle(uint8_t sec);
  void setKeepTime(uint32_t millisec);
  void setTimeStart(uint32_t millisec);
  bool getPinState();
  uint8_t getState();
  uint8_t getPin();
};

/*
 * For read DIP switch from port
 * define PORT pin in arrary
 *
 * Constructor parameter
 * *portfamily: used PORTxPIN define pin array in pin_constant.h
 * group: used PORTFx define port map value in constant.h
 *
 */
class SWx
{
private:
  uint8_t _portPin[8];
  uint8_t _portGroup;

public:
  /*
   * Constructor parameter
   * *portfamily: used PORTxPIN define pin array in pin_constant.h
   * group: used PORTFx define port map value in constant.h
   *
   */
  SWx(const uint8_t *portfamily, uint8_t group);
  SWx();
  /*
   * *portfamily: used PORTxPIN define pin array in pin_constant.h
   * group: used PORTFx define port map value in constant.h
   *
   */
  void Init(const uint8_t *portfamily, uint8_t group);
  /*
   * Read value of PORTx
   *
   */
  uint8_t getValue();
};
