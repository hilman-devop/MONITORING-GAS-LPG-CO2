# Sistem Monitoring Gas LPG & CO2

![Project Banner](https://via.placeholder.com/800x400.png?text=Sistem+Monitoring+Gas+LPG+%26+CO2)  
*Gambar ilustrasi sistem (ganti dengan foto asli perangkat jika ada)*

## Deskripsi Proyek

Proyek ini adalah sistem monitoring kadar gas LPG (menggunakan sensor MQ-6) dan CO2/kualitas udara (menggunakan sensor MQ-135) berbasis Arduino Mega dengan konektivitas WiFi melalui ESP-01 dan integrasi IoT menggunakan Blynk.

Sistem dilengkapi dengan:
- Pembacaan real-time kadar gas dalam PPM
- Alarm bertingkat (Aman → Waspada → Bahaya) dengan indikator LED, buzzer, dan suara dari DFPlayer Mini
- Kontrol relay otomatis
- Monitoring jarak jauh melalui aplikasi Blynk
- Kalibrasi otomatis (auto-baseline) dengan penyimpanan data di EEPROM

Metode khusus:
- **MQ-6**: Pendekatan linear berdasarkan tegangan dengan offset otomatis
- **MQ-135**: Metode eksponensial menggunakan library MQUnifiedsensor

## Fitur Utama

- Pembacaan sensor MQ-6 dan MQ-135 secara bersamaan
- Auto-baseline kalibrasi selama 3 menit saat pertama kali dijalankan atau melalui tombol di Blynk
- Indikator status koneksi Blynk (LED hijau berkedip jika terputus)
- Alarm suara berbeda untuk setiap level bahaya
- Kontrol manual relay melalui Blynk
- Recalibrate manual via aplikasi Blynk

## Hardware yang Dibutuhkan

- Arduino Mega 2560
- ESP-01 (ESP8266) sebagai WiFi shield
- Sensor MQ-6 (pin analog A3)
- Sensor MQ-135 (pin analog A0)
- DFPlayer Mini + microSD card (berisi file MP3 peringatan)
- LED Hijau (pin 2), Kuning (pin 3), Merah (pin 4)
- Buzzer aktif (pin 5)
- Relay module 2 channel (pin 6 & 7)
- Power supply stabil (5V untuk Arduino, 3.3V untuk sensor jika diperlukan)

## Library yang Digunakan

- `ESP8266_Lib.h`
- `BlynkSimpleShieldEsp8266.h`
- `SoftwareSerial.h`
- `DFRobotDFPlayerMini.h`
- `MQUnifiedsensor.h`
- `EEPROM.h`

## Instalasi & Penggunaan

1. **Upload kode** ke Arduino Mega menggunakan Arduino IDE.
2. **Ganti credential** di bagian atas kode:
   ```cpp
   char ssid[] = "YOUR_SSID";
   char pass[] = "YOUR_PASSWORD";
   #define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN_HERE"

Siapkan aplikasi Blynk:
Buat project baru
Tambahkan widget:
Value Display → V0 (LPG PPM)
Value Display → V1 (CO2 PPM)
Labeled Value → V2 (Status)
Button → V4 (Switch, mode: Switch) untuk kontrol relay
Button → V5 (Push) untuk recalibrate


Nyalakan sistem di udara bersih untuk auto-baseline pertama kali.
Untuk recalibrate manual, tekan tombol di V5 pada aplikasi Blynk.

## Catatan Penting

Pastikan sensor sudah dipanaskan minimal 24 jam sebelum kalibrasi akurat.
File MP3 di DFPlayer:
001.mp3 → Peringatan Bahaya
002.mp3 → Peringatan Waspada
003.mp3 → Status Aman

Jangan commit file dengan credential asli ke repository publik.

## Kontributor
Proyek ini dikerjakan oleh dua orang:

Hilman – Pengembangan kode, logika software, integrasi Blynk, dan metode kalibrasi linear MQ-6
Zidny – Perakitan hardware, wiring, pengujian perangkat, dan integrasi fisik sensor serta modul

Terima kasih kepada teman saya atas kerja kerasnya di bagian hardware!
