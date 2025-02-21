#include "Config.h"

//==========================================================================
void ColorSet(struct color *CS, uint8_t _color)
{
  switch (_color)
  {
  case RED:
    CS->R = 255;
    CS->G = 0;
    CS->B = 0;
    CS->hex = 0xFF0000;
    break;

  case GREEN:
    CS->R = 0;
    CS->G = 255;
    CS->B = 0;
    CS->hex = 0x00FF00;
    break;

  case BLUE:
    CS->R = 0;
    CS->G = 0;
    CS->B = 255;
    CS->hex = 0x0000FF;
    break;

  case BLACK:
    CS->R = 0;
    CS->G = 0;
    CS->B = 0;
    CS->hex = 0x000000;
    break;

  case WHITE:
    CS->R = 255;
    CS->G = 255;
    CS->B = 255;
    CS->hex = 0xFFFFFF;
    break;

  case SEA_WAVE:
    CS->R = 0;
    CS->G = 255;
    CS->B = 255;
    CS->hex = 0x00FFFF;
    break;

  case YELLOW:
    CS->R = 255;
    CS->G = 255;
    CS->B = 0;
    CS->hex = 0xFFFF00;
    break;

  case PURPLE:
    CS->R = 255;
    CS->G = 0;
    CS->B = 255;
    CS->hex = 0xFF00FF;
    break;

  default:
    break;
  }
}
//=======================================================================

int GetColorNum(struct color *C)
{
  int _col = 0;

  if (C->R == 255 && C->G == 0 && C->B == 0)
    _col = RED;
  if (C->R == 0 && C->G == 255 && C->B == 0)
    _col = GREEN;
  if (C->R == 0 && C->G == 0 && C->B == 255)
    _col = BLUE;
  if (C->R == 0 && C->G == 0 && C->B == 0)
    _col = BLACK;
  if (C->R == 255 && C->G == 255 && C->B == 255)
    _col = WHITE;
  if (C->R == 0 && C->G == 255 && C->B == 255)
    _col = SEA_WAVE;
  if (C->R == 255 && C->G == 255 && C->B == 0)
    _col = YELLOW;
  if (C->R == 255 && C->G == 0 && C->B == 255)
    _col = PURPLE;

  return _col;
}
//=======================================================================
// Writing color in RGB to buf
void ColorWrite(char *buf, struct color *C)
{
  strcat(buf, "<R>");
  itoa(C->R, buf + strlen(buf), DEC);
  strcat(buf, "</R>\r\n");
  strcat(buf, "<G>");
  itoa(C->G, buf + strlen(buf), DEC);
  strcat(buf, "</G>\r\n");
  strcat(buf, "<B>");
  itoa(C->B, buf + strlen(buf), DEC);
  strcat(buf, "</B>\r\n");
}
//=======================================================================

//=========================================================================
// Get WC Door State
bool GetWCState(uint8_t num)
{
  boolean state = false;
  switch (num)
  {
  case WC1:
    state = digitalRead(WC1);
    break;
  case WC2:
    state = digitalRead(WC2);
    break;

  default:
    break;
  }
  HCONF.WCSS == SENSOR_OPEN ? state : state = !state;
  return state;
}
//=========================================================================

//=========================================================================
void SetColorWC()
{
  switch (HCONF.WCL)
  {
  case NORMAL:
    if (STATE.SensWC1)
    {
      STATE.StateWC1 = true;
      ColorSet(&col_wc1, RED);
    }
    else
    {
      STATE.StateWC1 = false;
      ColorSet(&col_wc1, GREEN);
    }

    if (STATE.SensWC2)
    {
      STATE.StateWC2 = true;
      ColorSet(&col_wc2, RED);
    }
    else
    {
      STATE.StateWC2 = false;
      ColorSet(&col_wc2, GREEN);
    }
    break;

  case REVERSE:
    if (STATE.SensWC1)
    {
      STATE.StateWC2 = true;
      ColorSet(&col_wc2, RED);
    }
    else
    {
      STATE.StateWC2 = false;
      ColorSet(&col_wc2, GREEN);
    }

    if (STATE.SensWC2)
    {
      STATE.StateWC1 = true;
      ColorSet(&col_wc1, RED);
    }
    else
    {
      STATE.StateWC1 = false;
      ColorSet(&col_wc1, GREEN);
    }
    break;

  case ONE_HALL:
    if (HCONF.WCSS)
    {
      if (STATE.SensWC1 && STATE.SensWC2)
      {
        STATE.StateWC1 = true;
        STATE.StateWC2 = true;
        ColorSet(&col_wc1, RED);
        ColorSet(&col_wc2, RED);
      }
      else
      {
        STATE.StateWC1 = false;
        STATE.StateWC2 = false;
        ColorSet(&col_wc1, GREEN);
        ColorSet(&col_wc2, GREEN);
      }
    }
    else
    {
      if (!STATE.SensWC1 && !STATE.SensWC2)
      {
        STATE.StateWC1 = false;
        STATE.StateWC2 = false;
        ColorSet(&col_wc1, GREEN);
        ColorSet(&col_wc2, GREEN);
      }
      else
      {
        STATE.StateWC1 = true;
        STATE.StateWC2 = true;
        ColorSet(&col_wc1, RED);
        ColorSet(&col_wc2, RED);
      }
    }
    break;

  default:
    break;
  }
}
//=========================================================================

//=========================================================================
boolean SerialNumConfig()
{
  bool sn = false;
  if (CFG.sn == 0)
  {
    Serial.println("Serial number: FALL");
    Serial.println("Please enter SN");
    sn = true;
  }

  while (sn)
  {
    if (Serial.available())
    {

      digitalWrite(LED_ST, HIGH);

      String r = Serial.readString();
      bool block_st = false;
      r.trim();
      CFG.sn = r.toInt();

      digitalWrite(LED_ST, LOW);

      if (CFG.sn >= 1)
      {
        Serial.printf("Serial number: %d DONE \r\n", CFG.sn);
        sn = false;
        return true;
      }

      log_i("free heap=%i", ESP.getFreeHeap());
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }
  return false;
}
//=========================================================================

//=======================================================================
void UserPresetInit()
{
  ColorSet(&col_carnum, YELLOW);
  ColorSet(&col_wc1, GREEN);
  ColorSet(&col_speed, WHITE);
  ColorSet(&col_time, WHITE);
  ColorSet(&col_date, WHITE);
  ColorSet(&col_tempin, GREEN);
  ColorSet(&col_tempout, BLUE);

  UserText.carnum = 77;

  strcat(UserText.carname, "Вагон ");
}

/**************************************** Scanning I2C bus *********************************************/
void I2C_Scanning(void)
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 8; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);
}
/*******************************************************************************************************/

/************************ System Initialisation **********************/
void SystemInit(void)
{
  STATE.IDLE = true;
  STATE.StaticUPD = true;
  STATE.DynamicUPD = true;
  STATE.DUPDBlock = false;
  STATE.LedRS = false;
  STATE.LedWiFi = false;
  STATE.SaveFlash = false;
  STATE.RS = false;
  STATE.Debug = true;
  STATE.CurDebug = false;
  STATE.WiFiEnable = true;
  STATE.TTS = false;   // Flag Start Time Speech
  STATE.DSWC = false;  // Flag Start WC Speech
  STATE.DSTS1 = false; // Flag Start WC_1 Speech
  STATE.DSTS2 = false; // Flag Start WC_2 Speech
  STATE.VolumeUPD = false;

  GetChipID();
}
/*******************************************************************************************************/

/***************************** Function Show information or Device *************************************/
void ShowInfoDevice(void)
{
  Serial.println(F("Starting..."));
  Serial.println(F("TableController_0845"));
  Serial.print(F("SN:"));
  Serial.println(CFG.sn);
  Serial.print(F("fw_date:"));
  Serial.println(CFG.fwdate);
  Serial.println(CFG.fw);
  Serial.println(CFG.chipID);
  Serial.println(F("by EmbedDev"));
  Serial.println();
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void GetChipID()
{
  uint32_t chipId = 0;

  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  CFG.chipID = chipId;
}
/*******************************************************************************************************/
/*******************************************************************************************************/
String GetMacAdr()
{
  CFG.MacAdr = WiFi.macAddress(); //
  Serial.print(F("MAC:"));        // временно
  Serial.println(CFG.MacAdr);     // временно
  return WiFi.macAddress();
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Debug information
void DebugInfo()
{
#ifndef DEBUG
  if (STATE.Debug)
  {
    char message[37];

    Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));

    sprintf(message, "StaticUPD: %0d CNT: %0d", STATE.StaticUPD, STATE.cnt_Supd);
    Serial.println(message);
    sprintf(message, "CAR_NUM: %d HideMode: %d", UserText.carnum, UserText.hide_t);
    Serial.println(message);
    Serial.printf("Brightness:");
    Serial.println(HCONF.bright);
    sprintf(message, "GMT: %d", CFG.gmt);
    Serial.println(message);
    sprintf(message, "RTC Time: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
    Serial.println(message);
    sprintf(message, "RTC Date: %4d.%02d.%02d", Clock.year, Clock.month, Clock.date);
    Serial.println(message);
    sprintf(message, "T1: %0.1f T2: %0.1f", HCONF.dsT1, HCONF.dsT2);
    Serial.println(message);
    sprintf(message, "Get_WC_from: %d ", HCONF.WCGS);
    Serial.println(message);

    if (HCONF.WCGS == 1)
    {
      sprintf(message, "WC_STAT: %d", STATE.StateWC1);
    }
    else
    {
      sprintf(message, "WC_STAT: %d", STATE.StateWC2);
    }

    Serial.println(message);

    if (HCONF.ADR == 1)
    {
      sprintf(message, "T1_OFS: %d T2_OFS: %d", HCONF.T1_offset, HCONF.T2_offset);
      Serial.println(message);
      sprintf(message, "WC1 | Sensor: %d State %d", STATE.SensWC1, STATE.StateWC1);
      Serial.println(message);
      sprintf(message, "Color_WC1 %00006X", col_wc1.hex);
      Serial.println(message);
      sprintf(message, "WC2 | Sensor: %d State %d", STATE.SensWC2, STATE.StateWC2);
      Serial.println(message);
      sprintf(message, "Color_WC2 %00006X", col_wc2.hex);
      Serial.println(message);
    }
    sprintf(message, "VOL: %d", HCONF.volume);
    Serial.println(message);

    // Serial.printf("SN:");
    // Serial.println(CFG.sn);

    Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
    Serial.println();
  }
#endif
}

//=======================================================================

/*******************************************************************************************************/
void ShowFlashSave()
{
  bool state = false;

  for (uint8_t i = 0; i <= 1; i++)
  {
    digitalWrite(LED_ST, ON);
    vTaskDelay(400 / portTICK_PERIOD_MS);
    digitalWrite(LED_ST, OFF);
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}
/***************************************************** ****************************/
void SystemFactoryReset()
{
  CFG.gmt = 3;
  CFG.WiFiMode = AccessPoint;
  CFG.APSSID = "0845";
  CFG.APPAS = "retra0845zxc";
  CFG.IP1 = 192;
  CFG.IP2 = 168;
  CFG.IP3 = 1;
  CFG.IP4 = 1;
  CFG.GW1 = 192;
  CFG.GW2 = 168;
  CFG.GW3 = 1;
  CFG.GW4 = 1;
  CFG.MK1 = 255;
  CFG.MK2 = 255;
  CFG.MK3 = 255;
  CFG.MK4 = 0;

  HCONF.bright = 90;
  HCONF.volume = 21;
  HCONF.T1_offset = 0;
  HCONF.T2_offset = 0;
  HCONF.WCL = NORMAL;
  HCONF.WCSS = SENSOR_CLOSE;
  HCONF.WCGS = 1;

  STATE.WiFiEnable = true;

  ColorSet(&col_carnum, WHITE);
  ColorSet(&col_wc1, GREEN);
  ColorSet(&col_wc2, GREEN);
  ColorSet(&col_time, WHITE);
  ColorSet(&col_date, WHITE);
  ColorSet(&col_tempin, GREEN);
  ColorSet(&col_tempout, BLUE);

  UserText.hide_t = false;
  UserText.carnum = 7;

  memset(UserText.carname, 0, strlen(UserText.carname));
  strcat(UserText.carname, "Вагон ");
}
