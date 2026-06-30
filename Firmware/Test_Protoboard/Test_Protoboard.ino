/*
 * IGNIS-1 — Test de protoboard (bring-up de hardware)
 * --------------------------------------------------------------
 * NO es el firmware de vuelo. Solo verifica que cada periferico
 * responde, con el pin-map del diseno. Cada subsistema imprime
 * PASS/FAIL y se puede desactivar si el modulo no esta conectado.
 *
 * Pin-map (ESP32 DevKit 38p):
 *   I2C  SDA/SCL ......... 33 / 32   (BME280 0x76, MPU6050 0x68)
 *   SPI  SCK/MISO/MOSI ... 18 / 19 / 23   (LoRa + microSD, CS separados)
 *   LoRa CS/RST/DIO0 ..... 5 / 14 / 26   (RFM95W 433 MHz)
 *   microSD CS ........... 13
 *   MPU_INT .............. 27
 *   Buzzer ............... 25   (activo)
 *   LED status ........... 4
 *   GPS  RX2/TX2 ......... 16 / 17   (NEO-6M, 9600 baud)
 *   MQ-135 A0 ............ 35   (ADC1, via divisor 10k/15k)
 *
 * Librerias (Library Manager):
 *   Adafruit BME280, Adafruit MPU6050, Adafruit Unified Sensor,
 *   TinyGPSPlus, LoRa (Sandeep Mistry), SD (incluida).
 *
 * Notas:
 *   - MQ-135 necesita calentamiento (varios minutos); valores
 *     inestables al inicio. El divisor 10k/15k baja A0 (0-5V) a ~0-3V.
 *   - Alimentar MQ-135 y GPS a 5V; ESP/sensores I2C a 3.3V.
 */

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <TinyGPSPlus.h>
#include <SD.h>
#include <LoRa.h>

// ---------- activar/desactivar pruebas ----------
#define TEST_BME280   1
#define TEST_MPU6050  1
#define TEST_MQ135    1
#define TEST_GPS      1
#define TEST_SD       0   // sin modulo microSD (no comprado aun)
#define TEST_LORA     0   // sin modulo RFM95W (no comprado aun)

// ---------- pines ----------
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
#define LED       4
#define GPS_RX    16    // ESP RX2 <- GPS TX
#define GPS_TX    17    // ESP TX2 -> GPS RX
#define MQ_AOUT   35    // ADC1 (input-only)
#define LORA_FREQ 433E6

// divisor MQ-135: A0 --[10k]--+--[15k]-- GND ; tap = ADC
const float MQ_DIV = (10.0 + 15.0) / 15.0;   // factor inverso = 1.667

Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;

bool okBME=false, okMPU=false, okSD=false, okLoRa=false;
uint32_t seq=0;

void beep(int ms){ digitalWrite(BUZZER,HIGH); delay(ms); digitalWrite(BUZZER,LOW); }
void line(){ Serial.println(F("------------------------------------------")); }

void i2cScan(){
  Serial.println(F("[I2C] scan:"));
  int n=0;
  for(uint8_t a=1;a<127;a++){
    Wire.beginTransmission(a);
    if(Wire.endTransmission()==0){
      Serial.printf("   0x%02X", a);
      if(a==0x76||a==0x77) Serial.print(F(" (BME280)"));
      if(a==0x68||a==0x69) Serial.print(F(" (MPU6050)"));
      Serial.println();
      n++;
    }
  }
  Serial.printf("   %d dispositivo(s)\n", n);
}

void setup(){
  Serial.begin(115200);
  delay(500);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(MPU_INT, INPUT);
  digitalWrite(LED,HIGH); beep(120); digitalWrite(LED,LOW);

  line();
  Serial.println(F("IGNIS-1 — Test de protoboard"));
  line();

  Wire.begin(I2C_SDA, I2C_SCL);
  i2cScan();

#if TEST_BME280
  okBME = bme.begin(0x76) || bme.begin(0x77);
  Serial.printf("[BME280]  %s\n", okBME ? "PASS" : "FAIL (revisar I2C/dir)");
#endif

#if TEST_MPU6050
  okMPU = mpu.begin(0x68);
  if(okMPU){
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  }
  Serial.printf("[MPU6050] %s\n", okMPU ? "PASS" : "FAIL");
#endif

#if TEST_MQ135
  analogReadResolution(12);
  analogSetPinAttenuation(MQ_AOUT, ADC_11db);   // rango ~0-3.1V
  Serial.println(F("[MQ-135]  ADC listo (necesita warmup)"));
#endif

#if TEST_GPS
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println(F("[GPS]     UART2 abierto (esperando NMEA)"));
#endif

#if (TEST_SD || TEST_LORA)
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
#endif

#if TEST_SD
  okSD = SD.begin(SD_CS, SPI);
  Serial.printf("[microSD] %s\n", okSD ? "PASS" : "FAIL");
  if(okSD){
    File f = SD.open("/test.txt", FILE_APPEND);
    if(f){ f.println("IGNIS-1 test boot"); f.close(); Serial.println(F("   escribio /test.txt")); }
  }
#endif

#if TEST_LORA
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  okLoRa = LoRa.begin(LORA_FREQ);
  if(okLoRa) LoRa.setSyncWord(0x12);
  Serial.printf("[LoRa]    %s (433 MHz)\n", okLoRa ? "PASS" : "FAIL");
#endif

  line();
  Serial.println(F("Loop: lecturas cada 2 s"));
  line();
  if(okBME && okMPU){ beep(60); delay(60); beep(60); }  // doble beep = sensores I2C OK
}

void loop(){
  digitalWrite(LED, !digitalRead(LED));   // latido

  // alimentar parser GPS continuamente
#if TEST_GPS
  while(Serial2.available()) gps.encode(Serial2.read());
#endif

  Serial.println();

#if TEST_BME280
  if(okBME){
    Serial.printf("BME280:  T=%.2f C  RH=%.1f %%  P=%.1f hPa\n",
                  bme.readTemperature(), bme.readHumidity(), bme.readPressure()/100.0);
  }
#endif

#if TEST_MPU6050
  if(okMPU){
    sensors_event_t a,g,tmp; mpu.getEvent(&a,&g,&tmp);
    Serial.printf("MPU6050: ax=%.2f ay=%.2f az=%.2f m/s2 | gz=%.2f rad/s\n",
                  a.acceleration.x, a.acceleration.y, a.acceleration.z, g.gyro.z);
  }
#endif

#if TEST_MQ135
  {
    int mv = analogReadMilliVolts(MQ_AOUT);     // mV en el ADC (tras divisor)
    float a0 = mv/1000.0 * MQ_DIV;              // V reales en A0 del modulo
    Serial.printf("MQ-135:  ADC=%d mV  A0~=%.2f V  (warmup necesario)\n", mv, a0);
  }
#endif

#if TEST_GPS
  if(gps.location.isValid()){
    Serial.printf("GPS:     %.6f, %.6f  sats=%d  alt=%.1f m\n",
                  gps.location.lat(), gps.location.lng(),
                  gps.satellites.value(), gps.altitude.meters());
  } else {
    Serial.printf("GPS:     sin fix (chars=%lu, sats=%d)\n",
                  (unsigned long)gps.charsProcessed(), gps.satellites.value());
  }
#endif

#if TEST_LORA
  if(okLoRa){
    LoRa.beginPacket();
    LoRa.printf("IGNIS-1 test seq=%lu", (unsigned long)seq);
    LoRa.endPacket();
    Serial.printf("LoRa:    TX seq=%lu\n", (unsigned long)seq);
    seq++;
  }
#endif

  delay(2000);
}
