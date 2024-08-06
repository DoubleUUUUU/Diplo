#include "sensor_reader.h"

SensorReader::SensorReader(const int sensorPin): dht(sensorPin, DHT_TYPE) {
    this->sensorPin = sensorPin;
    dht.begin();
};

SensorData SensorReader::readSensorData() {
    return {
        temperature: this->dht.readTemperature(),
        humidity: this->dht.readHumidity(),
        heatIndex: this->dht.computeHeatIndex(),
    };
};