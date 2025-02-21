#ifndef _RS485_H
#define _RS485_H

#include <Arduino.h>
#include "Config.h"
#include "SharedParserFunctions.hpp"

void RS485_ReadADR();
bool parseBuffer(char *buffer, uint16_t crc, uint32_t length);
void getTimeChar(char *array);
void getDateChar(char *array);
void Send_GPSdata();
void Send_ITdata(uint8_t adr);
void Send_BSdata();
void Send_BS_UserData(char *msg1, char *msg2);
unsigned int CRC16_mb(char *buf, int len);
uint16_t calcCRC(char *str, uint32_t len);

#endif