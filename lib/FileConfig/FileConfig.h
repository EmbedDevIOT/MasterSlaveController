#ifndef FileConfig_H
#define FileConfig_H

#include "Config.h"
#include <AT24Cxx.h>

void LoadConfig();
void SaveConfig();
void ShowLoadJSONConfig();
void EEP_Write();
void EEP_Read();

#endif