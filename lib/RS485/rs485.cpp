#include "rs485.h"

uint16_t gps_new_crc;

extern SemaphoreHandle_t i2c_mutex;

//=======================================================================
void RS485_ReadADR()
{
    if (digitalRead(SW1) && digitalRead(SW2))
    {
        HCONF.ADR = 0;
    }
    else if (!digitalRead(SW1) && digitalRead(SW2))
    {
        HCONF.ADR = 1;
        pinMode(DE_RE, OUTPUT);
        digitalWrite(DE_RE, HIGH);
    }
    else if (digitalRead(SW1) && !digitalRead(SW2))
    {
        HCONF.ADR = 2;
    }
    else if (!digitalRead(SW1) && !digitalRead(SW2))
    {
        HCONF.ADR = 3;
        pinMode(DE_RE, OUTPUT);
        digitalWrite(DE_RE, LOW);
    }

    Serial.print("\r\n");
    Serial.println(F("##############  RS CONFIGURATION  ###############"));
    Serial.printf("ADR: %d \r\n", HCONF.ADR);
    Serial.println(F("#################################################"));
    Serial.print("\r\n");

    vTaskDelay(500 / portTICK_PERIOD_MS);
    for (uint8_t i = 1; i <= HCONF.ADR; i++)
    {
        digitalWrite(LED_ST, HIGH);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        digitalWrite(LED_ST, LOW);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}
//=======================================================================

//=======================================================================
int getTimeGMT(uint8_t hour, int gmt)
{
    int TimGMT = hour + gmt;

    if (TimGMT < 0)
    {
        TimGMT = hour + (24 - CFG.gmt);
    }
    else if (TimGMT > 24)
    {
        TimGMT = TimGMT - 24;
    }
    else if (TimGMT == 24)
    {
        TimGMT = 0;
    }

    return TimGMT;
}
//=======================================================================

//=======================================================================
// buffer - recieving UART buffer pointer
bool parseBuffer(char *buffer, uint16_t crc, uint32_t length)
{
    Buf_t xmlBuf;
    bool result;
    int tmp = 0;
    double temp_speed = 0;

    std::string auxtext1;

    struct TMP
    {
        bool wc = false;
        uint8_t vol = 0;
        int timezone = 0;
        int ds1 = 0;
        int ds2 = 0;
        uint8_t sec = 0;
        uint8_t min = 0;
        uint8_t hour = 0;
        uint8_t date = 0;
        uint8_t month = 0;
        uint16_t year = 0; // 1..2999
        int c_speed;       // km/h
    } buff;

    auto bufferEnd = buffer + length;

    while (reinterpret_cast<uint32_t>(buffer) < reinterpret_cast<uint32_t>(bufferEnd))
    {
        // Get line from buffer
        if (!extractLineAndParseXml(&buffer, bufferEnd, xmlBuf))
        {
            return false;
        }

        if (xmlBuf.tag == "gps_data")
        {
            // general tag
            continue;
        }

        // Stop processing on the end tag
        if (xmlBuf.end_tag == "gps_data")
        {
            if (2020 <= buff.year && buff.year <= 2060)
            {
                // Serial.printf("CALC CRC: %04x \r\n", crc);
                // Serial.printf("NEW CRC: %04x \r\n", gps_new_crc);

                if (crc == gps_new_crc)
                {
                    Serial.printf("CRC = OK \r\n");

                    int timGMT = getTimeGMT(buff.hour, buff.timezone);

                    xSemaphoreTake(i2c_mutex, portMAX_DELAY);
                    RTC.setTime(buff.sec, buff.min, timGMT,
                                buff.date, buff.month, buff.year);
                    xSemaphoreGive(i2c_mutex);

                    HCONF.dsT1 = buff.ds1;
                    HCONF.dsT2 = buff.ds2;
                    STATE.WC = buff.wc;
                    CFG.gmt = buff.timezone;
                    if (HCONF.volume != buff.vol)
                    {
                        HCONF.volume = buff.vol;
                        STATE.VolumeUPD = true;
                    }
                }
                else
                {
                    Serial.printf("CRC = ERROR \r\n");
                }
                // clock.SetFullTimeGPS(hour, min, sec, timezone, day, month, year);
                return true;
            }
            else
                return false;
        }

        // Here starts specific device parameters parsing
        if (xmlBuf.tag == "gmt")
        {
            buff.timezone = static_cast<int>(strtol(xmlBuf.value.c_str(), nullptr, 10));
        }
        else if (xmlBuf.tag == "time")
        {
            tmp = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
            buff.sec = (tmp % 100);
            buff.min = (tmp / 100) % 100;
            buff.hour = (tmp / 10000);
            // Serial.printf("Time: %d:%d:%d", Clock.hour, Clock.minute, Clock.second);
        }
        else if (xmlBuf.tag == "date")
        {
            tmp = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
            buff.date = tmp / 10000;
            buff.month = (tmp / 100) % 100;
            buff.year = 2000 + (tmp % 100); // 2000 is added cause yy
            // Serial.printf("Time: %d:%d:%d", Clock.date, Clock.month, Clock.year);
        }
        else if (xmlBuf.tag == "lat")
        {
            // skip
        }
        else if (xmlBuf.tag == "lon")
        {
            // skip
        }
        else if (xmlBuf.tag == "speed")
        {
            temp_speed = strtod(xmlBuf.value.c_str(), NULL);

            buff.c_speed = (int)(temp_speed * 1.852);
        }
        else if (xmlBuf.tag == "temp1")
        {

            if (xmlBuf.value[0] != 'N')
            {
                buff.ds1 = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
            }
        }
        else if (xmlBuf.tag == "temp2")
        {
            if (xmlBuf.value[0] != 'N')
            {
                {
                    buff.ds2 = static_cast<int>(strtoul(xmlBuf.value.c_str(), nullptr, 10));
                }
            }
        }
        // else if (xmlBuf.tag == "auxtext1")
        // {
        //     auxtext1 = xmlBuf.value;
        // }
        else if (xmlBuf.tag == "wc")
        {
            buff.wc = static_cast<int>(strtol(xmlBuf.value.c_str(), nullptr, 10));
        }
        else if (xmlBuf.tag == "vol_dis1")
        {
            buff.vol = static_cast<int>(strtol(xmlBuf.value.c_str(), nullptr, 10));
        }
        else if (xmlBuf.tag == "gps_crc")
        {
            gps_new_crc = static_cast<int>(strtol(xmlBuf.value.c_str(), nullptr, 16));
        }
    }

    return false;
}
//==========================================================================

//==========================================================================
void getTimeChar(char *array)
{
    uint8_t hour = Clock.hour;

    int TimGMT = hour - CFG.gmt;

    // Serial.printf("TimGMT1: %d \r\n", TimGMT);

    if (TimGMT < 0)
    {
        TimGMT = hour + (24 - CFG.gmt);
    }
    else if (TimGMT > 24)
    {
        TimGMT = TimGMT - 24;
    }
    else if (TimGMT == 24)
    {
        TimGMT = 0;
    }

    // Serial.printf("TimGMT2: %d \r\n", TimGMT);

    array[0] = TimGMT / 10 + '0';
    array[1] = TimGMT % 10 + '0';
    array[2] = Clock.minute / 10 + '0';
    array[3] = Clock.minute % 10 + '0';
    array[4] = Clock.second / 10 + '0';
    array[5] = Clock.second % 10 + '0';
    array[6] = '\0';

    // for (uint8_t i = 0; i <= 6; i++)
    // {
    //   Serial.print(array[i]);
    // }
    // Serial.print("\r\n");
}
//==========================================================================

//==========================================================================
void getDateChar(char *array)
{
    // DateTime now = RTC.getTime();

    int8_t bytes[4];
    byte amount;
    uint16_t year;
    year = Clock.year;

    for (byte i = 0; i < 4; i++)
    {                         //>
        bytes[i] = year % 10; // записываем остаток в буфер
        year /= 10;           // "сдвигаем" число
        if (year == 0)
        {               // если число закончилось
            amount = i; // запомнили, сколько знаков
            break;
        }
    } // массив bytes хранит цифры числа data в обратном порядке!

    array[0] = Clock.date / 10 + '0';
    array[1] = Clock.date % 10 + '0';
    array[2] = Clock.month / 10 + '0';
    array[3] = Clock.month % 10 + '0';
    array[4] = (char)bytes[1] + '0';
    array[5] = (char)bytes[0] + '0';
    array[6] = '\0';
}
//==========================================================================

//========================== Sending GPS Protocol  =========================
void Send_GPSdata()
{
    getTimeChar(CFG.time);
    getDateChar(CFG.date);

    unsigned int crc = 0;

    char buf_crc[256] = "";
    char xml[256] = "";

    memset(buf_crc, 0, strlen(buf_crc));
    memset(xml, 0, strlen(xml));

    if (CFG.gmt <= 0)
    {
        strcat(buf_crc, "<gmt>");
    }
    else
        strcat(buf_crc, "<gmt>+");

    itoa(CFG.gmt, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</gmt>\r\n");
    strcat(buf_crc, "<time>");
    strcat(buf_crc, CFG.time);
    strcat(buf_crc, "</time>\r\n");
    strcat(buf_crc, "<date>");
    strcat(buf_crc, CFG.date);
    strcat(buf_crc, "</date>\r\n");
    strcat(buf_crc, "<lat></lat>\r\n");
    strcat(buf_crc, "<lon></lon>\r\n");
    strcat(buf_crc, "<speed></speed>\r\n");

    strcat(buf_crc, "<temp1>");
    if (HCONF.dsT1 <= -100 or HCONF.dsT1 == 85)
    {
        strcat(buf_crc, "N/A");
        strcat(buf_crc, "</temp1>\r\n");
    }
    else
    {
        itoa(HCONF.dsT1, buf_crc + strlen(buf_crc), DEC);
        strcat(buf_crc, "</temp1>\r\n");
    }

    strcat(buf_crc, "<temp2>");
    if (HCONF.dsT2 <= -100 or HCONF.dsT2 == 85)
    {
        strcat(buf_crc, "N/A");
        strcat(buf_crc, "</temp2>\r\n");
    }
    else
    {
        itoa(HCONF.dsT2, buf_crc + strlen(buf_crc), DEC);
        strcat(buf_crc, "</temp2>\r\n");
    }

    strcat(buf_crc, "<wc>");
    if (HCONF.WCGS == 1)
    {
        itoa(STATE.StateWC1, buf_crc + strlen(buf_crc), DEC);
    }
    else
    {
        itoa(STATE.StateWC2, buf_crc + strlen(buf_crc), DEC);
    }
    strcat(buf_crc, "</wc>\r\n");

    strcat(buf_crc, "<vol_dis1>");
    itoa(HCONF.volume, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</vol_dis1>\r\n");

    crc = CRC16_mb(buf_crc, strlen(buf_crc));

    strcat(xml, "<gps_data>\r\n");
    strcat(xml, buf_crc);
    strcat(xml, "<gps_crc>");
    char crc_temp[5] = {0};
    sprintf(crc_temp, "%04X", crc);
    strcat(xml, crc_temp);
    strcat(xml, "</gps_crc>\r\n");
    strcat(xml, "</gps_data>");

    Serial2.print(xml);
    Serial2.println();
}
//=========================================================================

//========================== Sending IT Protocol  =========================
// Extended Board
void Send_ITdata(uint8_t adr)
{
    unsigned int crc;
    char buf_crc[8000] = "";
    memset(buf_crc, 0, strlen(buf_crc));
    char xml[8000] = "";
    memset(xml, 0, strlen(xml));

    strcat(buf_crc, "<adr id=\"1\">\r\n");
    strcat(buf_crc, "<cabin_mode>1</cabin_mode>\r\n");
    strcat(buf_crc, "<TsizeDx>\r\n");
    strcat(buf_crc, "<X>128</X>\r\n");
    strcat(buf_crc, "<Y>64</Y>\r\n");
    strcat(buf_crc, "</TsizeDx>\r\n");
    strcat(buf_crc, "<TsizeMx>\r\n");
    strcat(buf_crc, "<X>2</X>\r\n");
    strcat(buf_crc, "<Y>2</Y>\r\n");
    strcat(buf_crc, "</TsizeMx>\r\n");
    strcat(buf_crc, "<TsizeLx>\r\n");
    strcat(buf_crc, "<X>256</X>\r\n");
    strcat(buf_crc, "<Y>128</Y>\r\n");
    strcat(buf_crc, "</TsizeLx>\r\n");
    strcat(buf_crc, "<rgb>1</rgb>\r\n");
    strcat(buf_crc, "<rgb_balance>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</rgb_balance>\r\n");
    strcat(buf_crc, "<min_bright>");
    itoa(HCONF.bright, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</min_bright>\r\n");
    strcat(buf_crc, "<max_bright>100</max_bright>\r\n");
    strcat(buf_crc, "<auto_bright>0</auto_bright>\r\n");
    strcat(buf_crc, "<zones>6</zones>\r\n");
    strcat(buf_crc, "\r\n");

    //=============== ZONE 1 ================
    strcat(buf_crc, "<zone id=\"1\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>1</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>74</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Carnum Color
    // ColorWrite(buf_crc, &col_carnum);
    // Carnum color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>");
    strcat(buf_crc, "$textbs");
    strcat(buf_crc, "</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 2 ================
    strcat(buf_crc, "<zone id=\"2\">\r\n");
    strcat(buf_crc, "<startX>80</startX>\r\n");
    strcat(buf_crc, "<startY>1</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>49</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Time Color
    ColorWrite(buf_crc, &col_time);
    // Time Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$time</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 3 ================
    strcat(buf_crc, "<zone id=\"3\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>17</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>74</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    ColorWrite(buf_crc, &col_carnum);
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0");
    strcat(buf_crc, "</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<direction>RTL</direction>\r\n");
    strcat(buf_crc, "<speed>10");
    strcat(buf_crc, "</speed>\r\n");
    strcat(buf_crc, "<text>");
    strcat(buf_crc, "$route");
    strcat(buf_crc, "</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 4 ================
    strcat(buf_crc, "<zone id=\"4\">\r\n");
    strcat(buf_crc, "<startX>80</startX>\r\n");
    strcat(buf_crc, "<startY>17</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>49</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Date Color Start
    ColorWrite(buf_crc, &col_date);
    // Date Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$dd $mm1</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 5 ================
    strcat(buf_crc, "<zone id=\"5\">\r\n");
    strcat(buf_crc, "<startX>80</startX>\r\n");
    strcat(buf_crc, "<startY>26</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>24</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // TempIN Color Start
    ColorWrite(buf_crc, &col_tempin);
    // TempIN Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$temp1°C</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 5 ================
    strcat(buf_crc, "<zone id=\"6\">\r\n");
    strcat(buf_crc, "<startX>105</startX>\r\n");
    strcat(buf_crc, "<startY>26</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>24</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // TempOUT Color Start
    ColorWrite(buf_crc, &col_tempout);
    // TempOUT Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$temp2°C</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    // strcat(buf_crc, "\r\n");
    //=======================================
    strcat(buf_crc, "</adr>\r\n");
    //========= adress 1 end=================
    strcat(buf_crc, "\r\n");

    strcat(buf_crc, "<adr id=\"2\">\r\n");
    strcat(buf_crc, "<cabin_mode>1</cabin_mode>\r\n");
    strcat(buf_crc, "<TsizeDx>\r\n");
    strcat(buf_crc, "<X>128</X>\r\n");
    strcat(buf_crc, "<Y>64</Y>\r\n");
    strcat(buf_crc, "</TsizeDx>\r\n");
    strcat(buf_crc, "<TsizeMx>\r\n");
    strcat(buf_crc, "<X>2</X>\r\n");
    strcat(buf_crc, "<Y>2</Y>\r\n");
    strcat(buf_crc, "</TsizeMx>\r\n");
    strcat(buf_crc, "<TsizeLx>\r\n");
    strcat(buf_crc, "<X>256</X>\r\n");
    strcat(buf_crc, "<Y>128</Y>\r\n");
    strcat(buf_crc, "</TsizeLx>\r\n");
    strcat(buf_crc, "<rgb>1</rgb>\r\n");
    strcat(buf_crc, "<rgb_balance>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</rgb_balance>\r\n");
    strcat(buf_crc, "<min_bright>");
    itoa(HCONF.bright, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</min_bright>\r\n");
    strcat(buf_crc, "<max_bright>100</max_bright>\r\n");
    strcat(buf_crc, "<auto_bright>0</auto_bright>\r\n");
    strcat(buf_crc, "<zones>6</zones>\r\n");
    strcat(buf_crc, "\r\n");

    //=============== ZONE 1 ================
    strcat(buf_crc, "<zone id=\"1\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>1</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>74</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Carnum Color
    // ColorWrite(buf_crc, &col_carnum);
    // Carnum color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>");
    strcat(buf_crc, "$textbs");
    strcat(buf_crc, "</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 2 ================
    strcat(buf_crc, "<zone id=\"2\">\r\n");
    strcat(buf_crc, "<startX>80</startX>\r\n");
    strcat(buf_crc, "<startY>1</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>49</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Time Color
    ColorWrite(buf_crc, &col_time);
    // Time Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$time</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 3 ================
    strcat(buf_crc, "<zone id=\"3\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>17</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>74</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    ColorWrite(buf_crc, &col_carnum);
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0");
    strcat(buf_crc, "</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<direction>RTL</direction>\r\n");
    strcat(buf_crc, "<speed>10");
    strcat(buf_crc, "</speed>\r\n");
    strcat(buf_crc, "<text>");
    strcat(buf_crc, "$route");
    strcat(buf_crc, "</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 4 ================
    strcat(buf_crc, "<zone id=\"4\">\r\n");
    strcat(buf_crc, "<startX>80</startX>\r\n");
    strcat(buf_crc, "<startY>17</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>49</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Date Color Start
    ColorWrite(buf_crc, &col_date);
    // Date Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$dd $mm1</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 5 ================
    strcat(buf_crc, "<zone id=\"5\">\r\n");
    strcat(buf_crc, "<startX>80</startX>\r\n");
    strcat(buf_crc, "<startY>26</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>24</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // TempIN Color Start
    ColorWrite(buf_crc, &col_tempin);
    // TempIN Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$temp1°C</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================

    //=============== ZONE 5 ================
    strcat(buf_crc, "<zone id=\"6\">\r\n");
    strcat(buf_crc, "<startX>105</startX>\r\n");
    strcat(buf_crc, "<startY>26</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>24</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // TempOUT Color Start
    ColorWrite(buf_crc, &col_tempout);
    // TempOUT Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$temp2°C</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    //=======================================
    strcat(buf_crc, "</adr>\r\n");
    strcat(buf_crc, "\r\n");
    //========= adress 2 end=================

    //========= adress 3 START=================
    strcat(buf_crc, "<adr id=\"3\">\r\n");
    strcat(buf_crc, "<cabin_mode>1</cabin_mode>\r\n");
    strcat(buf_crc, "<TsizeDx>\r\n");
    strcat(buf_crc, "<X>64</X>\r\n");
    strcat(buf_crc, "<Y>32</Y>\r\n");
    strcat(buf_crc, "</TsizeDx>\r\n");
    strcat(buf_crc, "<TsizeMx>\r\n");
    strcat(buf_crc, "<X>1</X>\r\n");
    strcat(buf_crc, "<Y>1</Y>\r\n");
    strcat(buf_crc, "</TsizeMx>\r\n");
    strcat(buf_crc, "<TsizeLx>\r\n");
    strcat(buf_crc, "<X>256</X>\r\n");
    strcat(buf_crc, "<Y>128</Y>\r\n");
    strcat(buf_crc, "</TsizeLx>\r\n");
    strcat(buf_crc, "<rgb>1</rgb>\r\n");
    strcat(buf_crc, "<rgb_balance>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</rgb_balance>\r\n");
    strcat(buf_crc, "<min_bright>");
    itoa(HCONF.bright, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</min_bright>\r\n");
    strcat(buf_crc, "<max_bright>100</max_bright>\r\n");
    strcat(buf_crc, "<auto_bright>0</auto_bright>\r\n");
    strcat(buf_crc, "<zones>4</zones>\r\n");
    strcat(buf_crc, "\r\n");
    //=============== ZONE 1 ================
    strcat(buf_crc, "<zone id=\"1\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>1</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>64</X>\r\n");
    strcat(buf_crc, "<Y>16</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Time Color
    ColorWrite(buf_crc, &col_time);
    // Time Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>");
    strcat(buf_crc, "$time");
    strcat(buf_crc, "</text>\r\n");
    strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================
    //=============== ZONE 2 ================
    strcat(buf_crc, "<zone id=\"2\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>16</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>64</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    ColorWrite(buf_crc, &col_wc1);
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$route</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================
    //=============== ZONE 3 ================
    strcat(buf_crc, "<zone id=\"3\">\r\n");
    strcat(buf_crc, "<startX>1</startX>\r\n");
    strcat(buf_crc, "<startY>25</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>32</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    ColorWrite(buf_crc, &col_tempin);
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0");
    strcat(buf_crc, "</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>");
    strcat(buf_crc, "$temp1°C");
    strcat(buf_crc, "</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    strcat(buf_crc, "\r\n");
    //=======================================
    //=============== ZONE 4 ================
    strcat(buf_crc, "<zone id=\"4\">\r\n");
    strcat(buf_crc, "<startX>33</startX>\r\n");
    strcat(buf_crc, "<startY>25</startY>\r\n");
    strcat(buf_crc, "<size>\r\n");
    strcat(buf_crc, "<X>32</X>\r\n");
    strcat(buf_crc, "<Y>8</Y>\r\n");
    strcat(buf_crc, "</size>\r\n");
    strcat(buf_crc, "<color>\r\n");
    // Date Color Start
    ColorWrite(buf_crc, &col_tempout);
    // Date Color END
    strcat(buf_crc, "</color>\r\n");
    strcat(buf_crc, "<bgcolor>\r\n");
    strcat(buf_crc, "<R>0</R>\r\n");
    strcat(buf_crc, "<G>0</G>\r\n");
    strcat(buf_crc, "<B>0</B>\r\n");
    strcat(buf_crc, "</bgcolor>\r\n");
    strcat(buf_crc, "<mode>0</mode>\r\n");
    strcat(buf_crc, "<align>center</align>\r\n");
    strcat(buf_crc, "<text>$temp2°C</text>\r\n");
    strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
    strcat(buf_crc, "</zone>\r\n");
    //=======================================
    strcat(buf_crc, "</adr>\r\n");
    strcat(buf_crc, "\r\n");
    //========= Table ADR 3 end =================
    Serial.printf("size BUFF: %d", strlen(buf_crc));

    crc = CRC16_mb(buf_crc, strlen(buf_crc));
    strcat(xml, "<extboard_data>\r\n");
    strcat(xml, buf_crc);
    strcat(xml, "<extboard_crc>");

    char crc_temp[5] = {0};
    sprintf(crc_temp, "%04X", crc);
    strcat(xml, crc_temp);
    // itoa(crc, xml + strlen(xml), HEX);
    strcat(xml, "</extboard_crc>\r\n");
    strcat(xml, "</extboard_data>");
    Serial2.println(xml);
    Serial2.println();
    // Serial.printf("size XML: %d", strlen(xml));
}
//=========================================================================

//========================== Sending BS Protocol  =========================
// Internal Board
void Send_BSdata()
{
    // memset(UserText.carname, 0, strlen(UserText.carname));
    unsigned int crc = 0;

    char buf[1256] = "";
    char xml[1256] = "";

    // ADR 1 START
    strcat(buf, "<adr id=\"1\">\r\n");
    strcat(buf, "<color_text>");
    char buf_col[9] = {0};

    sprintf(buf_col, "%00006X", col_wc1.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_text>\r\n");
    strcat(buf, "<color_date>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_date.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_date>\r\n");
    strcat(buf, "<color_day>FF0000</color_day>\r\n");
    // memset(buf_col, 0, 7);
    // sprintf(buf_col, "%00006X", col_day.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_day>\r\n");
    strcat(buf, "<color_time>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_time.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_time>\r\n");
    strcat(buf, "<color_temp_in>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempin.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_in>\r\n");
    strcat(buf, "<color_temp_out>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempout.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_out>\r\n");
    strcat(buf, "<color_route>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_carnum.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_route>\r\n");
    strcat(buf, "<color_speed>FF0000");
    strcat(buf, "</color_speed>\r\n");
    strcat(buf, "<route_name>");
    if (UserText.hide_t == false)
    {
        strcat(buf, UserText.carname);
        strcat(buf, " ");
        itoa(UserText.carnum, buf + strlen(buf), DEC);
    }
    else
    {
        strcat(buf, UserText.carname);
    }
    strcat(buf, "</route_name>\r\n");
    strcat(buf, "<text_bs>Туалет");
    strcat(buf, "</text_bs>\r\n");
    strcat(buf, "</adr>\r\n");
    // ADR 1 END

    // ADR 2 START
    strcat(buf, "<adr id=\"2\">\r\n");
    strcat(buf, "<color_text>");

    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_wc2.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_text>\r\n");
    strcat(buf, "<color_date>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_date.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_date>\r\n");
    strcat(buf, "<color_day>FF0000</color_day>\r\n");
    // memset(buf_col, 0, 7);
    // sprintf(buf_col, "%00006X", col_day.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_day>\r\n");
    strcat(buf, "<color_time>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_time.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_time>\r\n");
    strcat(buf, "<color_temp_in>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempin.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_in>\r\n");
    strcat(buf, "<color_temp_out>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempout.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_out>\r\n");
    strcat(buf, "<color_route>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_carnum.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_route>\r\n");
    strcat(buf, "<color_speed>FF0000");
    strcat(buf, "</color_speed>\r\n");
    strcat(buf, "<route_name>");
    if (UserText.hide_t == false)
    {
        strcat(buf, UserText.carname);
        strcat(buf, " ");
        itoa(UserText.carnum, buf + strlen(buf), DEC);
    }
    else
    {
        strcat(buf, UserText.carname);
    }
    strcat(buf, "</route_name>\r\n");
    strcat(buf, "<text_bs>Туалет");
    strcat(buf, "</text_bs>\r\n");
    strcat(buf, "</adr>\r\n");
    // ADR 2 END

    // ADR 3 START
    strcat(buf, "<adr id=\"3\">\r\n");
    // strcat(buf, "<color_text>");

    // memset(buf_col, 0, 9);
    // sprintf(buf_col, "%00006X", col_wc1.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_text>\r\n");
    // strcat(buf, "<color_date>");
    // memset(buf_col, 0, 9);
    // sprintf(buf_col, "%00006X", col_date.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_date>\r\n");
    // strcat(buf, "<color_day>FF0000</color_day>\r\n");
    // memset(buf_col, 0, 7);
    // sprintf(buf_col, "%00006X", col_day.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_day>\r\n");
    strcat(buf, "<color_time>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_time.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_time>\r\n");
    strcat(buf, "<color_temp_in>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempin.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_in>\r\n");
    strcat(buf, "<color_temp_out>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempout.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_out>\r\n");
    strcat(buf, "<color_route>");
    memset(buf_col, 0, 9);

        
    if (HCONF.WCGS == 1)
    {
        sprintf(buf_col, "%00006X", col_wc1.hex);
    }
    else
    {
        sprintf(buf_col, "%00006X", col_wc2.hex);
    }

    strcat(buf, buf_col);
    strcat(buf, "</color_route>\r\n");
    // strcat(buf, "<color_speed>FF0000");
    // strcat(buf, "</color_speed>\r\n");
    strcat(buf, "<route_name>Туалет");
    // if (UserText.hide_t == false)
    // {
    //     strcat(buf, UserText.carname);
    //     strcat(buf, " ");
    //     itoa(UserText.carnum, buf + strlen(buf), DEC);
    // }
    // else
    // {
    //     strcat(buf, UserText.carname);
    // }
    strcat(buf, "</route_name>\r\n");
    // strcat(buf, "<text_bs>Туалет");
    // strcat(buf, "</text_bs>\r\n");
    strcat(buf, "</adr>\r\n");
    // ADR 2 END

    // CRC
    crc = CRC16_mb(buf, strlen(buf));
    strcat(xml, "<intboard_data>\r\n");
    strcat(xml, buf);
    strcat(xml, "<intboard_crc>");
    char crc_temp[5] = {0};
    sprintf(crc_temp, "%04X", crc);
    strcat(xml, crc_temp);
    strcat(xml, "</intboard_crc>\r\n");
    strcat(xml, "</intboard_data>");
    strcat(xml, "\r\n");

    Serial2.print(xml);
    Serial2.println();
}
//=========================================================================

//======================== Sending BS user message ==========================
// msg1 - TOP zone  (YELLOW)
// msg2 - BOT zone  (WHITE)
void Send_BS_UserData(char *msg1, char *msg2)
{
    unsigned int crc = 0;

    char buf[1024] = "";
    char xml[1024] = "";

    strcat(buf, "<adr id=\"1\">\r\n");

    strcat(buf, "<color_text>");
    char buf_col[9] = {0};

    sprintf(buf_col, "%00006X", 0xFFFF00);
    strcat(buf, buf_col);
    strcat(buf, "</color_text>\r\n");
    strcat(buf, "<color_date>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_date.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_date>\r\n");
    strcat(buf, "<color_day>FF0000</color_day>\r\n");
    // memset(buf_col, 0, 7);
    // sprintf(buf_col, "%00006X", col_day.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_day>\r\n");
    strcat(buf, "<color_time>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_time.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_time>\r\n");
    strcat(buf, "<color_temp_in>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempin.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_in>\r\n");
    strcat(buf, "<color_temp_out>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempout.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_out>\r\n");
    strcat(buf, "<color_route>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", 0xFFFFFF);
    strcat(buf, buf_col);
    strcat(buf, "</color_route>\r\n");
    strcat(buf, "<color_speed>FF0000");
    strcat(buf, "</color_speed>\r\n");

    strcat(buf, "<route_name>");
    strcat(buf, msg2);
    strcat(buf, "</route_name>\r\n");

    strcat(buf, "<text_bs>");
    strcat(buf, msg1);
    strcat(buf, "</text_bs>\r\n");

    strcat(buf, "</adr>\r\n");

    strcat(buf, "<adr id=\"2\">\r\n");
    strcat(buf, "<color_text>");

    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", 0xFFFF00);
    strcat(buf, buf_col);
    strcat(buf, "</color_text>\r\n");
    strcat(buf, "<color_date>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_date.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_date>\r\n");
    strcat(buf, "<color_day>FF0000</color_day>\r\n");
    // memset(buf_col, 0, 7);
    // sprintf(buf_col, "%00006X", col_day.hex);
    // strcat(buf, buf_col);
    // strcat(buf, "</color_day>\r\n");
    strcat(buf, "<color_time>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_time.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_time>\r\n");
    strcat(buf, "<color_temp_in>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempin.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_in>\r\n");
    strcat(buf, "<color_temp_out>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", col_tempout.hex);
    strcat(buf, buf_col);
    strcat(buf, "</color_temp_out>\r\n");
    strcat(buf, "<color_route>");
    memset(buf_col, 0, 9);
    sprintf(buf_col, "%00006X", 0xFFFFFF);
    strcat(buf, buf_col);
    strcat(buf, "</color_route>\r\n");
    strcat(buf, "<color_speed>FF0000");
    strcat(buf, "</color_speed>\r\n");
    strcat(buf, "<route_name>");
    strcat(buf, msg2);
    strcat(buf, "</route_name>\r\n");
    strcat(buf, "<text_bs>");
    strcat(buf, msg1);
    strcat(buf, "</text_bs>\r\n");
    strcat(buf, "</adr>\r\n");

    // CRC
    crc = CRC16_mb(buf, strlen(buf));
    strcat(xml, "<intboard_data>\r\n");
    strcat(xml, buf);
    strcat(xml, "<intboard_crc>");
    char crc_temp[5] = {0};
    sprintf(crc_temp, "%04X", crc);
    strcat(xml, crc_temp);
    strcat(xml, "</intboard_crc>\r\n");
    strcat(xml, "</intboard_data>");
    strcat(xml, "\r\n");

    Serial2.print(xml);
    Serial2.println();
}
//=========================================================================

//======================= CRC Check summ calculators  =====================
unsigned int CRC16_mb(char *buf, int len)
{
    unsigned int crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned int)buf[pos]; // XOR byte into least sig. byte of crc

        for (int i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc & 0x0001) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= 0xA001;
                // crc ^= 0x8005;
            }
            else           // Else LSB is not set
                crc >>= 1; // Just shift right
        }
    }
    return crc;
}
//=========================================================================

//======================= CRC Check summ calculators  =====================
uint16_t calcCRC(char *str, uint32_t len)
{
    if (len < 53)
        return 0xFFFF;

    uint16_t crc = 0xffff;
    const uint16_t poly = 0xa001;
    uint32_t start_pos = 17;
    uint32_t crc_end = len - 53; // cutting end tags

    if (strstr(str, "gps_data"))
    {
        start_pos = 12; // 12
        crc_end = len - 38;
    }

    // BOM protection
    if ((str[0] == 0xEF) && (str[1] == 0xBB) && (str[2] == 0xBF))
    {
        start_pos += 3;
    }

    for (int pos = start_pos; pos < crc_end; ++pos)
    {
        crc ^= str[pos];
        for (int i = 8; i != 0; --i)
        {
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= poly;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}
//=========================================================================
