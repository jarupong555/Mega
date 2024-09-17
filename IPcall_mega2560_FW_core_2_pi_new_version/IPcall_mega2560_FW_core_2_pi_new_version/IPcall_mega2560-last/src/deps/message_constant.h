/*
 *  IPcall Nurse call service Firmware version 1.0.1
 *  For IPcall board Version m1.0.1, m1.1.2
 *
 */

const uint8_t CNT_PREFLIGHT = 6;

// length = 32 + deviceID length => 32 + 3 -> 35
// #define M_LEN_CALL 35
const char M_CALL_SETACTIVE[] = ":1/A+1/1 \"ALARMCALL\" \"WARD-1\" \""; // activate message to server
const char M_CALL_DEACTIVE[] = ":1/A-1/1 \"ALARMCALL\" \"WARD-1\" \"";  // deactivate message to server

// length = 26 + deviceID length => 26 + 3 -> 29
// #define M_LEN_SALINE 29
const char M_SALINE_SETACTIVE[] = ":1/A+1/1 \"LOW\" \"WARD-1\" \""; // saline low active message to server
const char M_SALINE_DEACTIVE[] = ":1/A-1/1 \"LOW\" \"WARD-1\" \"";  // saline normal message to server

// length = 5
// #define M_LEN_FLAG 5
const char M_CALLGREEN[] = "-001\""; // Call Green flag to server
const char M_CALLEMER[] = "-004\"";  // Call Emer flag to server
const char M_PENDANT[] = "-007\"";   // Pendant flag to server
const char M_PC_CALL[] = "-008\"";   // Pull cord flag to server
const char M_SALINE[] = "-010\"";    // saline flag to server
const char M_EXTERNAL[] = "-011\"";  // External flag to server
const char M_EXTERNAL1[] = "-012\""; // External1 flag to server
const char M_PC_LOSE[] = "-111\"";   // Pull cord lose flag to server

// For forwardODL message link to color code map(constant.h)
const char M_FODL[5][10] = {"red", "green", "pendant", "emer", "pullcord"};

const uint8_t PORT_CODE_ODL_Y = 5;
const uint8_t PORT_CODE_ODL_G = 7;
const uint8_t PORT_CODE_ODL_R = 24;
