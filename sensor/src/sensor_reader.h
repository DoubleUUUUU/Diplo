#pragma once

#include <sensor_data.h>
#include <DHT.h>

#define DHT_TYPE DHT22

class SensorReader {
    int sensorPin;

    DHT dht; 
public:
    SensorReader(const int sensorPin);
    SensorData readSensorData();
};

