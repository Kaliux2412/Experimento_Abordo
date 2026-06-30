/*
 * Experimento de vuelo Ignitia - Firmware ESP32 (DevKit WROOM-32)
 * Esqueleto de partida. Pines VERIFICADOS contra el esquemático KiCad (2026-06-23).
 *
 * Pin map ESP32:
 *   I2C SDA/SCL ............ 33 / 32   (BME280 0x76, MPU6050 0x68, MLX90614 0x5A, SGP30 0x58)
 *   SPI SCK/MISO/MOSI ...... 18 / 19 / 23   (compartido SD + LoRa, CS separados)
 *   LoRa CS/RST/DIO0 ....... 5 / 14 / 26   (RFM95W 433 MHz)
 *   microSD CS ............. 13
 *   MPU6050 INT ............ 27
 *   Buzzer ................. 25
 *   GPS RX2/TX2 (UART2) .... 16 / 17   (GPS_TX->RX2, ESP TX2->GPS_RX) @9600
 *
 * Librerias: Adafruit_BME280, Adafruit_MPU6050, Adafruit_MLX90614,
 *            Adafruit_SGP30, TinyGPSPlus, SD, SPI, LoRa (sandeepmistry).
 *
 * NOTA: esqueleto. Falta manejo de errores, watchdog, recuperacion de bus I2C,
 *       formato de paquete robusto y la logica final de "probabilidad de incendio".
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_SGP30.h>

// ---- Pines (verificados del esquematico) ----
#define I2C_SDA   33
#define I2C_SCL   32
#define SPI_SCK   18
#define SPI_MISO  19
#define SPI_MOSI  23
#define LORA_CS    5
#define LORA_RST  14
#define LORA_DIO0 26
#define SD_CS     13
#define MPU_INT   27
#define BUZZER    25
#define GPS_RX    16   // ESP RX2 <- GPS TX
#define GPS_TX    17   // ESP TX2 -> GPS RX

#define LORA_FREQ 433E6   // RFM95W 433 MHz

Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_SGP30 sgp;
TinyGPSPlus gps;

uint32_t lastTelem = 0, lastLog = 0;
bool sdOK = false, loraOK = false;

// ---- Indice de riesgo de incendio (a partir de T y HR del BME280) ----
float vpd_kPa(float T, float RH) {                 // deficit de presion de vapor
  float svp = 0.6108 * exp(17.27 * T / (T + 237.3));
  return svp * (1.0 - RH / 100.0);
}
float chandlerBurningIndex(float T, float RH) {    // CBI: >97 extremo
  return (((110.0 - 1.373 * RH) - 0.54 * (10.20 - T)) *
          (124.0 * pow(10, -0.0142 * RH))) / 60.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(MPU_INT, INPUT);

  Wire.begin(I2C_SDA, I2C_SCL);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Sensores I2C
  bme.begin(0x76);
  mpu.begin(0x68);
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);   // alto-g para lanzamiento
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mlx.begin();
  sgp.begin();                                     // ojo: warm-up ~15s

  // GPS en Serial2
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // microSD (CS=13)
  sdOK = SD.begin(SD_CS, SPI);

  // LoRa (CS=5, RST=14, DIO0=26)
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  loraOK = LoRa.begin(LORA_FREQ);
  LoRa.setSyncWord(0x12);
}

void readGPS() {
  while (Serial2.available()) gps.encode(Serial2.read());
}

void loop() {
  readGPS();

  // Lectura de sensores
  float T  = bme.readTemperature();
  float P  = bme.readPressure() / 100.0F;          // hPa
  float RH = bme.readHumidity();
  float altBaro = bme.readAltitude(1013.25);
  float irT = mlx.readObjectTempC();               // temp superficie (IR)

  sensors_event_t a, g, tmp;
  mpu.getEvent(&a, &g, &tmp);                       // accel/gyro

  if (sgp.IAQmeasure()) { /* sgp.TVOC, sgp.eCO2 */ }

  float vpd = vpd_kPa(T, RH);
  float cbi = chandlerBurningIndex(T, RH);

  // --- Log a SD (alta cadencia, blackbox) ---
  if (sdOK && millis() - lastLog > 200) {           // 5 Hz
    lastLog = millis();
    File f = SD.open("/flight.csv", FILE_APPEND);
    if (f) {
      f.printf("%lu,%.1f,%.2f,%.1f,%.1f,%.1f,%.2f,%u,%u,%.2f,%.1f,%.6f,%.6f,%.1f\n",
        millis(), T, P, RH, altBaro, irT,
        a.acceleration.z, sgp.TVOC, sgp.eCO2, vpd, cbi,
        gps.location.lat(), gps.location.lng(), gps.altitude.meters());
      f.close();
    }
  }

  // --- Telemetria LoRa (baja cadencia, paquete compacto) ---
  if (loraOK && millis() - lastTelem > 1000) {      // 1 Hz
    lastTelem = millis();
    LoRa.beginPacket();
    LoRa.printf("A=%.1f P=%.1f T=%.1f RH=%.0f CBI=%.0f LAT=%.5f LON=%.5f",
      altBaro, P, T, RH, cbi, gps.location.lat(), gps.location.lng());
    LoRa.endPacket();
  }
}
