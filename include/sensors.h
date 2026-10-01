#ifndef SENSORS_H
#define SENSORS_H

#include "globals.h"

void initSensors();
void handleLDR();
void handlePIR();
void handleDHT();
void handleFlame();
void handleMQ2();
void handleDoorSensor();

#endif