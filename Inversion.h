#pragma once
#include <Arduino.h>
#include "Bridage.h"

extern bool invertJoy[AX_COUNT];
extern bool invertPad[AX_COUNT];

void inversionLoadOrDefault();
void inversionSaveToEEPROM();
void inversionMountRoutes();
