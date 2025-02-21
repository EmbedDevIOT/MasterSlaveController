#ifndef _Config_H
#define _Config_H

#include <Arduino.h>

#include <Audio.h>
#include "HardwareSerial.h"
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
#include <microDS3231.h>
#include <ArduinoJson.h>
#include <EncButton.h>
// #include <ElegantOTA.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define UARTSpeed 115200
#define RSSpeed 115200

#define WiFi_
#define MAX_MENU  12

#define WiFiTimeON 10
#define Client 0
#define AccessPoint 1
#define WorkNET

#define debug(x) Serial.println(x)
// #define DEBUG
// #define I2C_SCAN

#define DISABLE 0
#define ENABLE 1

#define ON 1
#define OFF 0

//======================    G P I O        =====================
#define KBD_PIN 36 // Pin Analog Keyboard (NO USED)

#define LED_WiFi 13 // Led State WiFi Connection
#define LED_ST 12   // Status

#define T1 4 // Temperature sensor ds18b20 1
#define T2 2 // Temperature sensor ds18b20 2

#define WC1 39 // Pin status control WC 1
#define WC2 34 // Pin status control WC 2

// Switches 
#define SW1 18 // DIP Switch 1
#define SW2 19 // DIP Switch 2
// I2S
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25
#define I2S_SCK 28
// UART 1  - RS485 -
#define RS_SERIAL1
#ifdef RS_SERIAL1
#define TX1_PIN 33 // UART1_TX
#define RX1_PIN 32 // UART1_RX
#endif
// #define DE_RE 35 // DE_MAX13444
#define DE_RE 16 // DE_MAX13444

// UART 2 - GPS -
#define GPS_SERIAL2
#ifdef GPS_SERIAL2
#define TX2_PIN 17 // UART2_TX
#define RX2_PIN 16 // UART2_RX
#endif
//=======================================================================

//=======================================================================
extern MicroDS3231 RTC;
extern DateTime Clock;
//=======================================================================

//========================== ENUMERATION ================================
  
// MENU 
enum menu
{
  IDLE = 0,
  _CAR_NUM,
  _GMT,
  _MIN,
  _HOUR,
  _DAY,
  _MONTH,
  _YEAR,
  _BRIGHT,
  _VOL,
  _WCL,
  _WCSS,
  _WiFi
};

// COLOR DEFINITIONS
enum C
{
  RED = 0,
  GREEN,
  BLUE,
  SEA_WAVE,
  BLACK,
  WHITE,
  YELLOW,
  PURPLE
};

enum Clicks
{
  ONE = 1,
  TWO,
  THREE
};

enum DAY
{
  MON = 1,
  TUE,
  WED,
  THU,
  FRI,
  SAT,
  SUN
};

enum MONTH
{
  YAN = 1,
  FEB,
  MAR,
  APR,
  MAY,
  JUN,
  JUL,
  AUG,
  SEP,
  OCTB,
  NOV,
  DECM,
};

// WC_STATE_LOGIQ
enum _WCL
{
  NORMAL = 0,
  REVERSE,
  ONE_HALL
};

// WC_SENSOR_SIGNAL
enum _WCSS
{
  SENSOR_OPEN = 0,
  SENSOR_CLOSE,
};

enum BTN_VALUE
{
    bt5 = 4095,
    bt4 = 1920,
    bt3 = 1230,
    bt2 = 870,
    bt1 = 670
};


//=======================================================================

//=========================== GLOBAL CONFIG =============================
struct GlobalConfig
{
  uint16_t sn = 0;
  String fw = ""; // accepts from setup()
  String fwdate = "";
  String chipID = "";
  String MacAdr = "";
  String NTPServer = "pool.ntp.org";
  String APSSID = "0845";
  String APPAS = "retra0845zxc";

  String Ssid = "MkT";           // SSID Wifi network
  String Password = "QFCxfXMA3"; // Passwords WiFi network

  int gmt = 0;

  char time[7] = "";
  char date[9] = "";

  byte IP1 = 192;
  byte IP2 = 168;
  byte IP3 = 1;
  byte IP4 = 1;
  byte GW1 = 192;
  byte GW2 = 168;
  byte GW3 = 1;
  byte GW4 = 1;
  byte MK1 = 255;
  byte MK2 = 255;
  byte MK3 = 255;
  byte MK4 = 0;

  byte WiFiMode = AccessPoint; // Режим работы WiFi
};
extern GlobalConfig CFG;
//=======================================================================
struct UserData
{
  char carname[17] = "";
  char wcname[13] = "Tуалет";
  int carnum = 0;
  bool hide_t = false; // Hidden car number text
};
extern UserData UserText;

// Color config 
struct color
{
  int R;
  int G;
  int B;
  long hex;
};
extern color col_carnum;
extern color col_time;
extern color col_date;
extern color col_day;
extern color col_tempin;
extern color col_tempout;
extern color col_wc1;           // dynamic update
extern color col_wc2;           // dynamic update
extern color col_speed;     
//=======================================================================

//=======================================================================
struct HardwareConfig
{
  uint8_t ADR = 0;          // RS_ADR
  int8_t WCL = 0;           // WC_STATE_LOGIQ
  uint8_t WCGS = 1;         // WC_GET_SIGNAL
  uint8_t WCSS = 0;         // WC_SENSOR_SIGNAL
  uint8_t volume = 21;      // Volume  1...21
  int8_t bright = 70;       // Led Brightness 
  float dsT1 = 0.0;         // Temperature T1 
  int8_t T1_offset = 0;     // Temperature Offset T1 sensor
  float dsT2 = 0.0;         // Temperature T2 
  int8_t T2_offset = 0;     // Temperature Offset T2 sensor
  uint8_t ERRORcnt = 0;
};
extern HardwareConfig HCONF;
//=======================================================================

//=======================================================================
struct Flag
{
  bool I2C_Block = 0;
  bool StaticUPD : 1;
  uint8_t cnt_Supd = 0;
  bool DynamicUPD : 1;
  bool DUPDBlock : 1;
  bool IDLE : 1;
  bool LedWiFi : 1;
  bool LedRS : 1;
  bool LedUser : 1;
  bool SaveFlash : 1;
  bool RS : 1;
  bool Debug : 1;
  bool CurDebug : 1;
  bool WiFiEnable : 1;  
  bool TTS : 1;        // Time to speech
  bool DSWC : 1;       // WC Door State to speech
  bool DSTS1 : 1;      // WC 1 DoorState to speech
  bool DSTS2 : 1;      // WC 2 DoorState to speech
  bool VolumeUPD : 1;  // Volume update
  bool SensWC1 = 0;    // Sensor WC1 current state
  bool SensWC2 = 0;    // Sensor WC2 current state
  bool WC = 0;         // General State WC !!!
  bool StateWC1 = 0;   // General State WC_1 
  bool StateWC2 = 0;   // General State WC_2 
  uint8_t menu_tmr = 0;  
};
extern Flag STATE;
//============================================================================
//============================================================================
void ColorWrite(char *buf, struct color *C);
void ColorSet(struct color *CS, uint8_t _color);
int GetColorNum(struct color *C);
bool GetWCState(uint8_t num);
void SetColorWC(void);
boolean SerialNumConfig(void);
void UserPresetInit(void);
void SystemInit(void);     //  System Initialisation (variables and structure)
void I2C_Scanning(void);
void ShowInfoDevice(void); //  Show information or this Device
void GetChipID(void);
String GetMacAdr();
void DebugInfo(void);
void SystemFactoryReset(void);
void ShowFlashSave(void);
//============================================================================
#endif // _Config_H