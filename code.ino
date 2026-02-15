/*
 * SISTEM MONITORING GAS LPG & CO2
 * Board: Arduino Mega + ESP-01 (shield)
 * Sensor:
 *   - MQ-6  (LPG)  → Metode linear tegangan + offset otomatis
 *   - MQ-135 (CO2) → Library MQUnifiedsensor (eksponensial)
 * Fitur:
 *   - Blynk remote monitoring & control
 *   - DFPlayer Mini untuk voice alert
 *   - Auto baseline calibration (3 menit) + simpan ke EEPROM
 *   - Alarm bertingkat dengan LED, buzzer, relay
 */

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "......"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
// WARNING: Ganti dengan token Anda sendiri (jangan commit token asli ke repo publik)
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN_HERE"

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <MQUnifiedsensor.h>
#include <EEPROM.h>

// ==================== KONFIGURASI WIFI ====================
char ssid[] = "YOUR_SSID";      // Ganti dengan SSID Anda
char pass[] = "YOUR_PASSWORD";  // Ganti dengan password Anda

#define EspSerial Serial1
#define ESP8266_BAUD 115200
ESP8266 wifi(&EspSerial);

BlynkTimer timer;

// ==================== PIN DEFINITION ====================
const uint8_t MQ6_PIN      = A3;
const uint8_t MQ135_PIN    = A0;
const uint8_t LED_GREEN    = 2;
const uint8_t LED_YELLOW   = 3;
const uint8_t LED_RED      = 4;
const uint8_t BUZZER       = 5;
const uint8_t RELAY1       = 6;
const uint8_t RELAY2       = 7;

// DFPlayer Mini (SoftwareSerial pada pin 10, 11)
SoftwareSerial mp3Serial(10, 11);
DFRobotDFPlayerMini myDFPlayer;

// ==================== SENSOR MQ-135 ====================
#define BOARD_TYPE "Arduino Mega"
MQUnifiedsensor MQ135(BOARD_TYPE, 5.0, 10, MQ135_PIN, "MQ-135");  // Voltase referensi 5V untuk Mega
const float RATIO_MQ135_CLEAN_AIR = 3.6;

// ==================== KALIBRASI MQ-6 (LINEAR) ====================
const float BOARD_VREF      = 3.3;   // Tegangan referensi sensor MQikan (sensor MQ di 3.3V)
const float VCLEAN_MQ6      = 0.80;  // Tegangan di udara bersih (referensi)
const float VGAS_1000PPM    = 1.00;  // Tegangan pada ~1000 ppm (referensi)

struct CalData {
  float kCO2;           // Faktor kalibrasi CO2 (MQ-135)
  float vOffsetMQ6;     // Offset tegangan MQ-6 di udara bersih
  uint32_t magic;
};
CalData cal;
const uint32_t CAL_MAGIC = 0xA5C01235;

// ==================== UTILITAS ====================
float smoothVoltage(uint8_t pin, int samples = 8) {
  float sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(20);
  }
  return (sum / samples) / 1023.0 * BOARD_VREF;
}

float getPPM_MQ6(float voltage) {
  if (voltage <= cal.vOffsetMQ6) return 0.0;
  return (voltage - cal.vOffsetMQ6) * (1000.0 / (VGAS_1000PPM - VCLEAN_MQ6));
}

void saveCalibration() { EEPROM.put(0, cal); }

bool loadCalibration() {
  EEPROM.get(0, cal);
  return (cal.magic == CAL_MAGIC && cal.kCO2 > 0.01f && cal.kCO2 < 10000 && cal.vOffsetMQ6 > 0.1);
}

void autoBaseline() {
  Serial.println("[CAL] Auto baseline 3 menit dimulai. Jauhi sumber gas!");
  unsigned long start = millis();
  float sumCO2 = 0, sumVmq6 = 0;
  int count = 0;

  while (millis() - start < 180000UL) {  // 3 menit
    MQ135.update();
    float co2_raw = MQ135.readSensor();
    float v_mq6 = smoothVoltage(MQ6_PIN, 5);

    sumCO2 += co2_raw;
    sumVmq6 += v_mq6;
    count++;
    delay(150);
  }

  float avgCO2 = sumCO2 / count;
  cal.kCO2 = (avgCO2 > 0.1f) ? (400.0 / avgCO2) : 1.0f;
  cal.vOffsetMQ6 = sumVmq6 / count;
  cal.magic = CAL_MAGIC;
  saveCalibration();

  Serial.print("[CAL] kCO2 = "); Serial.println(cal.kCO2, 4);
  Serial.print("[CAL] MQ6 Offset = "); Serial.println(cal.vOffsetMQ6, 4);
}

void updateConnectionLED() {
  digitalWrite(LED_GREEN, Blynk.connected() ? HIGH : (millis() % 1000 < 500));
}

// ==================== SETUP ====================
int currentStatus = 0;

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("=== SISTEM MONITORING GAS - START ===");

  // ESP-01 init
  EspSerial.begin(ESP8266_BAUD);
  delay(1000);
  if (!EspSerial.find("OK")) {
    Serial.println("[ERROR] ESP-01 tidak merespons!");
    while (true) delay(1000);
  }
  Serial.println("[OK] ESP-01 siap");

  Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass);

  // Pin setup
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);

  // DFPlayer
  mp3Serial.begin(9600);
  if (myDFPlayer.begin(mp3Serial)) {
    myDFPlayer.volume(30);
    Serial.println("[OK] DFPlayer siap");
  } else {
    Serial.println("[WARN] DFPlayer tidak terdeteksi");
  }

  // MQ-135 init
  MQ135.setRegressionMethod(1);
  MQ135.setA(110.47); MQ135.setB(-2.86);
  MQ135.init();

  // Kalibrasi R0 MQ-135
  float r0 = 0;
  for (int i = 0; i < 40; i++) {
    MQ135.update();
    r0 += MQ135.calibrate(RATIO_MQ135_CLEAN_AIR);
    delay(300);
  }
  MQ135.setR0(r0 / 40);
  Serial.println("[OK] R0 MQ-135 selesai");

  // Load atau auto baseline
  if (!loadCalibration()) {
    autoBaseline();
  } else {
    Serial.println("[OK] Kalibrasi dimuat dari EEPROM");
  }

  timer.setInterval(500L, updateConnectionLED);
  Serial.println("=== SISTEM SIAP ===");
}

// ==================== BLYNK HANDLERS ====================
BLYNK_WRITE(V4) {  // Kontrol relay
  int state = param.asInt();
  digitalWrite(RELAY1, state ? LOW : HIGH);
  digitalWrite(RELAY2, state ? LOW : HIGH);
}

BLYNK_WRITE(V5) {  // Tombol recalibrate
  if (param.asInt() == 1) {
    autoBaseline();
    Blynk.virtualWrite(V5, 0);
  }
}

BLYNK_CONNECTED() {
  Serial.println("[BLYNK] Terhubung ke server");
}

// ==================== LOOP UTAMA ====================
void loop() {
  Blynk.run();
  timer.run();

  // Baca sensor MQ-6 (linear)
  float voltageMQ6 = smoothVoltage(MQ6_PIN, 8);
  float lpgPPM = getPPM_MQ6(voltageMQ6);

  // Baca sensor MQ-135 (eksponensial)
  float co2_raw = 0;
  for (int i = 0; i < 8; i++) {
    MQ135.update();
    co2_raw += MQ135.readSensor();
    delay(20);
  }
  float co2PPM = (co2_raw / 8) * cal.kCO2;
  co2PPM = constrain(co2PPM, 350, 5000);

  // Debug serial
  Serial.print("LPG: "); Serial.print(lpgPPM, 1);
  Serial.print(" ppm | CO2: "); Serial.print(co2PPM, 0);
  Serial.print(" ppm | MQ6 Voltage: "); Serial.println(voltageMQ6, 3);

  // Kirim ke Blynk
  Blynk.virtualWrite(V0, lpgPPM);
  Blynk.virtualWrite(V1, co2PPM);

  // Logika alarm
  int newStatus = 0;
  if (lpgPPM > 800 || co2PPM > 1000)       newStatus = 2;  // Bahaya
  else if (lpgPPM > 500 || co2PPM > 500)  newStatus = 1;  // Waspada

  if (newStatus != currentStatus) {
    currentStatus = newStatus;

    if (currentStatus == 2) {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      tone(BUZZER, 1000);
      myDFPlayer.play(1);  // Bahaya
      digitalWrite(RELAY1, LOW);
      digitalWrite(RELAY2, LOW);
      Blynk.virtualWrite(V2, "BAHAYA! Segera Evakuasi!");
    }
    else if (currentStatus == 1) {
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_RED, LOW);
      tone(BUZZER, 2000, 300); delay(400);
      tone(BUZZER, 2000, 300);
      myDFPlayer.play(2);  // Waspada
      digitalWrite(RELAY1, HIGH);
      digitalWrite(RELAY2, HIGH);
      Blynk.virtualWrite(V2, "WASPADA! Periksa Ruangan");
    }
    else {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      noTone(BUZZER);
      myDFPlayer.play(3);  // Aman
      digitalWrite(RELAY1, HIGH);
      digitalWrite(RELAY2, HIGH);
      Blynk.virtualWrite(V2, "Aman");
    }
  }

  delay(1000);
}