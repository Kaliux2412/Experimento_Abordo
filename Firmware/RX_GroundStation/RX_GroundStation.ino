/*
 * IGNIS-1 — Receptor / estacion terrena (LoRa RFM95W)
 * --------------------------------------------------------------
 * Recibe los paquetes del CanSat, mide RSSI y SNR, y calcula
 * perdida de paquetes usando el numero de secuencia "seq=N".
 * Entregable de la mision secundaria: calidad de enlace vs altitud.
 *
 * HW: ESP32 + RFM95W 433 MHz (mismo modulo que el CanSat).
 *   SPI SCK/MISO/MOSI ... 18 / 19 / 23
 *   LoRa CS/RST/DIO0 .... 5 / 14 / 26
 *   LED RX .............. 2 (LED onboard del DevKit; cambiar si hace falta)
 *
 * Debe coincidir con el TX: frecuencia 433 MHz y syncWord 0x12.
 *
 * Salida Serial: CSV ->  millis,seq,rssi_dBm,snr_dB,perdidos,perdida_%,payload
 * Copiala/logueala en la PC para graficar RSSI y perdida vs altitud
 * (la altitud viene en el payload del firmware de vuelo).
 *
 * Librerias: LoRa (Sandeep Mistry), SPI.
 */

#include <SPI.h>
#include <LoRa.h>

#define SPI_SCK   18
#define SPI_MISO  19
#define SPI_MOSI  23
#define LORA_CS    5
#define LORA_RST  14
#define LORA_DIO0 26
#define LED        2
#define LORA_FREQ 433E6
#define LORA_SYNC 0x12

// estadisticas de enlace
bool     haveFirst = false;
long     firstSeq  = 0;
long     lastSeq   = 0;
uint32_t rxCount   = 0;   // paquetes recibidos
long     lost      = 0;   // estimado por huecos en seq

long parseSeq(const String &s){
  int i = s.indexOf("seq=");
  if(i < 0) return -1;
  return s.substring(i + 4).toInt();
}

void setup(){
  Serial.begin(115200);
  delay(300);
  pinMode(LED, OUTPUT);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  Serial.println(F("IGNIS-1 — Receptor LoRa"));
  if(!LoRa.begin(LORA_FREQ)){
    Serial.println(F("[LoRa] FAIL — revisar cableado/antena"));
    while(true){ digitalWrite(LED,!digitalRead(LED)); delay(150); }
  }
  LoRa.setSyncWord(LORA_SYNC);
  Serial.println(F("[LoRa] PASS — escuchando 433 MHz"));
  Serial.println(F("millis,seq,rssi_dBm,snr_dB,perdidos,perdida_%,payload"));
}

void loop(){
  int sz = LoRa.parsePacket();
  if(sz == 0) return;

  // leer payload
  String payload = "";
  while(LoRa.available()) payload += (char)LoRa.read();
  int  rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  digitalWrite(LED, HIGH);

  rxCount++;
  long seq = parseSeq(payload);

  if(seq >= 0){
    if(!haveFirst){ haveFirst = true; firstSeq = seq; lastSeq = seq; }
    else if(seq > lastSeq){
      lost += (seq - lastSeq - 1);   // huecos entre el ultimo y este
      lastSeq = seq;
    }
    // (seq <= lastSeq -> repetido o desordenado: no cuenta como perdido)
  }

  long expected = haveFirst ? (lastSeq - firstSeq + 1) : rxCount;
  float lossPct = expected > 0 ? (100.0 * lost / expected) : 0.0;

  Serial.printf("%lu,%ld,%d,%.1f,%ld,%.1f,%s\n",
                (unsigned long)millis(), seq, rssi, snr, lost, lossPct, payload.c_str());

  digitalWrite(LED, LOW);
}
