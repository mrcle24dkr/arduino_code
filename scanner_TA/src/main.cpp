#include <Arduino.h>

// --- DEKLARASI FUNGSI (WAJIB DI PLATFORMIO) ---
void tampilOled(String a, String b);
void prosesAbsen(String qr);
String getJamSekarang(); // Fungsi baru untuk RTC

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WiFi.h>
#include <WiFiManager.h> 
#include <FirebaseESP32.h>
#include <ESP32QRCodeReader.h> 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"

// --- KONFIGURASI PIN BARU (Persiapan untuk SD Card) ---
#define SDA_PIN 12
#define SCL_PIN 13
#define BUTTON_PIN 4 

// --- OBJEK ---
Adafruit_SSD1306 display(128, 64, &Wire, -1);
RTC_DS3231 rtc; // Objek RTC
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WiFiManager wm;

// --- KONFIGURASI DATABASE ---
#define API_KEY "3x5VUfpdd4sNNv0BAUvxw81KP0LjPlXxR3NeFQr9"
#define DATABASE_URL "https://empirise-79f29-default-rtdb.asia-southeast1.firebasedatabase.app/" 

bool isSystemOn = true;       

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

  // 1. Inisialisasi Jalur I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // 2. Inisialisasi OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("OLED Error"); 
  }
  display.setRotation(2); 
  display.clearDisplay();
  display.setTextColor(WHITE);

  // 3. Inisialisasi RTC DS3231
  if (!rtc.begin()) {
    Serial.println("RTC Gagal Ditemukan!");
  } else {
    if (rtc.lostPower()) {
      Serial.println("RTC reset, mengatur ulang waktu...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  
  // 4. WiFi
  wm.setConfigPortalTimeout(180); 
  if (!wm.autoConnect("ABSENSI_CAM", "password123")) {
    ESP.restart();
  }
  tampilOled("WiFi Connected!", "Menunggu Kamera...");
  delay(500);

  // 5. Inisialisasi Kamera
  reader.setup(); 
  reader.begin();
  
  // Bangunkan OLED setelah kamera init
  delay(100); 
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(WHITE);

  // 6. Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = API_KEY;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  tampilOled("SIAP SCAN", "System Active");
  Serial.println("Sistem Siap! Arahkan QR Code.");
}

void loop() {
  if(digitalRead(BUTTON_PIN) == LOW) {
    // Tombol
  }

  if (isSystemOn) {
    struct QRCodeData qrCodeData;
    
    if (reader.receiveQrCode(&qrCodeData, 100)) {
      if (qrCodeData.valid) {
        String qrCode = (const char *)qrCodeData.payload;
        
        Serial.print("QR DITEMUKAN: ");
        Serial.println(qrCode);

        tampilOled("QR Terbaca!", "Kirim Data...");
        
        prosesAbsen(qrCode);
        
        delay(2000); 
        tampilOled("SIAP SCAN", "Arahkan Kamera");
      }
    }
    delay(10); 
  }
}

// --- FUNGSI MENGAMBIL WAKTU DARI RTC ---
String getJamSekarang() {
  DateTime now = rtc.now();
  char buffer[20];
  // Format: DD-MM-YYYY HH:MM:SS
  snprintf(buffer, sizeof(buffer), "%02d-%02d-%04d %02d:%02d:%02d", 
           now.day(), now.month(), now.year(), 
           now.hour(), now.minute(), now.second());
  return String(buffer);
}

void prosesAbsen(String qr) {
  // Cara aman VSC
  String pathUser = "/users/";
  pathUser += qr;
  
  Serial.print("Kirim ke Firebase... ");
  
  if (Firebase.getString(fbdo, pathUser)) {
    Serial.println("BERHASIL!");
    String namaUser = fbdo.stringData();
    
    // Cara aman VSC
    String pathLog = "/logs/";
    pathLog += String(millis()); 
    
    // AMBIL WAKTU ASLI DARI RTC
    String waktuAsli = getJamSekarang();
    
    FirebaseJson json;
    json.set("nama", namaUser);
    json.set("qr", qr);
    json.set("waktu", waktuAsli); // <--- MENGGANTIKAN "Auto"

    Firebase.setJSON(fbdo, pathLog, json);

    // Tampil Sukses di OLED beserta Jamnya
    display.clearDisplay();
    display.setCursor(0,0); display.setTextSize(2);
    display.println("HADIR!");
    display.setTextSize(1);
    display.println(namaUser);
    display.setCursor(0,50);
    display.println(waktuAsli); // Jam tampil di layar
    display.display();
    
  } else {
    Serial.println("GAGAL/TIDAK DIKENAL.");
    tampilOled("GAGAL", "QR Tidak Terdaftar");
  }
}

void tampilOled(String a, String b) {
  if (isSystemOn) {
    display.clearDisplay();
    display.setCursor(0,0); display.setTextSize(1);
    display.println(a);
    display.setCursor(0,20);
    display.println(b);
    display.display();
  }
}