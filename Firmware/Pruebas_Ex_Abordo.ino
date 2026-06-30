#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200);
//siempre va primero SDA y luego SCL
  Wire.begin(33, 32);

  if (!mlx.begin()) {
    Serial.println("No se detecto MLX90614");
    while (1);
  }
  if (!bme.begin(0x76)) {
    Serial.println("No se detecto BME280");
    while (1);
  }
  Serial.println("Sensor detectado");
}

void loop() {


  float tempAmb = mlx.readAmbientTempC();
  float tempObj = mlx.readObjectTempC();

  float pressure = bme.readPressure() / 100.0F;
  float humidity = bme.readHumidity();


  String json = "{";

  json += "\"temp_amb\":" + String(tempAmb, 2) + ",";
  json += "\"temp_obj\":" + String(tempObj, 2) + ",";
  json += "\"pressure\":" + String(pressure, 2) + ",";
  json += "\"humidity\":" + String(humidity, 2) + ",";

  json += "}";

  Serial.println(json);

  delay(1000);
}