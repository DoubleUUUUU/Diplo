#include "DHT.h"
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

//WIFI UND INFLUXDB CONNECTION

#define WIFI_SSID "HTL-Projekte"
#define WIFI_PASSWORD "Projekt2024"
#define INFLUXDB_URL "http://10.115.1.60:8086/"
#define INFLUXDB_TOKEN "cRVikbvvag0VEz-RmNm4tbo2OSzvJvdhaERPRfPPubEA8ro83-E1TesVgU1aK14PMXI7tffy9QEBKBU7CZGilw=="
#define INFLUXDB_ORG "HTLD"
#define INFLUXDB_BUCKET "HTLD"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point sensor("NeubauSensor");

//Set correct timezone
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// DHT22 Configuration
#define DHTPIN 4       // Digital pin connected to the DHT22 sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302), AM2321

#define DEBUG false

int wifi_status = -1;
float sen_tmp = 0.0; //DHT22 Temp
float sen_hum = 0.0; //SCD30 Hum
float sen_CO2 = 0.0; //SCD30 CO2
float SCD_tmp= 0.0;
long wifi_tries = 0;

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// SCD30 Configuration
SCD30 airSensor;

void setupWiFi()
{
  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  Serial.println("MAC: " + WiFi.macAddress());
  while (wifiMulti.run() != WL_CONNECTED)
  {
    wifi_status = 0;
    Serial.print(".");
    delay(1000);
    wifi_tries ++;
    Serial.print(wifi_tries);
    if (wifi_tries % 2 == 0)
      Serial.println("WIFI connecting ...");
    else  
      Serial.println("WIFI ... connecting ");
    if (wifi_tries > 3) // seems connection is not possible at the moment
      break; 
  }
  
  if (wifi_status > 0)
  {
    Serial.println();
    Serial.println("MAC: " + WiFi.macAddress());
    Serial.println("WIFI connected");
  }  

  if (wifi_status > 0)
  {
    Serial.println("TIME syncing ...");
    timeSync(TZ_INFO, "10.115.1.215", "time.nis.gov");
  }  

  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    wifi_status = 10;
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    wifi_status = 0;
  }
}

void writeData()
{
  sensor.clearFields();
  sensor.addField("humidity_Neubau", sen_hum);
  sensor.addField("temperature_Neubau", sen_tmp);
  sensor.addField("CO2_Neubau", sen_CO2);
  sensor.addField("Temperatur_NeubauSCD: SCD", SCD_tmp);

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("Wifi connection lost");
    wifi_status = 0;
    Serial.println("WIFI status"+ String(wifi_status));  
    delay(1000);
  }
  else if (!client.writePoint(sensor))
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    wifi_status = 1;
    Serial.println("WIFI status"+ String(wifi_status));  
    delay(1000);
  }
  else
    wifi_status = 10;
}

void measure() {
  sen_tmp= dht.readTemperature();
  sen_hum= airSensor.getHumidity();
  sen_CO2= airSensor.getCO2();
  SCD_tmp= airSensor.getTemperature();

  float humidity = dht.readHumidity();
  float temperatureC = sen_tmp;
  float temperatureF = dht.readTemperature(true);

  if (isnan(sen_tmp)) {
    Serial.println(F("Failed to read from DHT22 sensor!"));
  } else 
  {
    float heatIndexC = dht.computeHeatIndex(temperatureC, humidity, false);
    float heatIndexF = dht.computeHeatIndex(temperatureF, humidity);

    Serial.print(F("Temperatur: "));
    Serial.println(sen_tmp);
  }
  if (airSensor.dataAvailable()) {
    Serial.print("CO2: ");
    Serial.print(sen_CO2);
    Serial.println(" ppm");
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(sen_hum);
    Serial.println(" %");
    Serial.print("Temperatur-SCD:");
    Serial.println(SCD_tmp);
  } else {
    Serial.println("SCD30 -> No data available");
  }

  delay(2000);

}

void setup() {
  sensor.addTag("environment", "env_neubau_sensor");
  Serial.begin(115200);
  
  Serial.println(F("Initializing DHT22 sensor..."));
  dht.begin();
  
  Serial.println(F("Initializing SCD30 sensor..."));
  Wire.begin(21, 22);
  Wire.setClock(40000); //Frequenz
  
  if (airSensor.begin() == false) {
    Serial.println("SCD30 not detected. Please check wiring.");
    while (1);
  }
}

void loop() {
  measure();
  if(!DEBUG && wifi_status >0)
  {
    writeData();
    Serial.println("Wrote data");
  }
  if (wifi_tries > 3 || wifi_status < 10)
  {
    wifi_tries = 0;
    setupWiFi();
  }

  delay(2000);  // Wait 2 seconds before the next loop iteration
}
