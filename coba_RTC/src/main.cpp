#include <Arduino.h> // WAJIB DI PLATFORMIO
#include <Wire.h>
#include "RTClib.h"

// Membuat objek RTC DS3231
RTC_DS3231 rtc;

// Array untuk nama hari agar tampilan lebih bagus
char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};

void setup() {
  // Memulai komunikasi serial untuk melihat hasil di Serial Monitor
  Serial.begin(115200);
  
  // Memberi waktu agar Serial Monitor siap
  delay(1000); 
  Serial.println("--- Test Modul RTC DS3231 (PlatformIO) ---");

  // Memulai komunikasi I2C dan mengecek apakah modul RTC terdeteksi
  if (!rtc.begin()) {
    Serial.println("Gagal menemukan RTC DS3231!");
    Serial.println("Cek kembali kabel SDA dan SCL Anda.");
    while (1) delay(10); // Program berhenti di sini jika RTC tidak ketemu
  }

  // --- BAGIAN PENTING: SETTING WAKTU ---
  // Fungsi rtc.lostPower() mengecek apakah baterai koin pernah dicabut/habis
  if (rtc.lostPower()) {
    Serial.println("RTC kehilangan power! Mengatur waktu ke waktu kompilasi...");
    // Di PlatformIO, macro __DATE__ dan __TIME__ juga berfungsi sama untuk mengambil waktu komputer saat di-compile
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
    // --- OPSI MANUAL ---
    // rtc.adjust(DateTime(2026, 4, 16, 21, 54, 0)); // Tahun, Bulan, Tanggal, Jam, Menit, Detik
  } else {
    Serial.println("RTC berjalan normal. Waktu tidak di-reset.");
  }
}

void loop() {
  // Mengambil waktu terkini dari modul RTC
  DateTime now = rtc.now();

  // Menampilkan waktu ke Serial Monitor
  Serial.print("Tanggal & Waktu: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  // Jeda 1 detik sebelum membaca waktu lagi
  delay(1000);
}