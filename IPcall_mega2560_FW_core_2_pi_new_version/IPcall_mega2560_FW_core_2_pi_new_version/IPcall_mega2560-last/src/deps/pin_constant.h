/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

#define BUILT_IN_LED    13  // common Cathode, D13 <-> PB7

#define EX_CALL_SW      10  // ***
#define EX_CANCEL_SW    3   // ***
#define EX_LED          68  // ***

#define CALL_SW         12  // *
#define CALL_LED        69  // common Anode, D69 <-> PK7
#define CALL_CANCEL_SW  19  // *
#define CALL_CANCEL_LED 39  // common Anode, D39 <-> PG2

#define PENDANT_LED     6   // common Anode, D6 <-> PH6
// Board v1.0.1
// #define PENDANT_SW_OP   11  // *
// Board v1.0.1
#define PENDANT_SW_A0   54  // *
#define PENDANT_SW_A13  67  // *

#define LED_RED         38  // common Anode, D38 <-> PD7
#define LED_GREEN       41  // common Anode, D41 <-> PG0
#define LED_YELLOW      40  // common Anode, D40 <-> PG1

#define ODL_RED         64  // *
#define ODL_GREEN       65  // *
#define ODL_YELLOW      66  // *

#define PULLCORD_LED    63  // **
#define PULLCORD_SW     62  // **
#define PULLCORD_CANCEL 18  // **

#define BUZZER          4   // common Cathode, D4 <-> PG5

// // SW 7 port const
const uint8_t PORTAPIN[8] = {29, 28, 27, 26, 25, 24, 23, 22};
// // SW 8 port const
const uint8_t PORTCPIN[8] = {37, 36, 35, 34, 33, 32, 31, 30};
// // SW 9 port const
const uint8_t PORTLPIN[8] = {49, 48, 47, 46, 45, 44, 43, 42};
