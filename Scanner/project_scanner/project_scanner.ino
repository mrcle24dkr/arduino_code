#include "soc/soc.h"             // Library untuk kontrol daya (Brownout)
#include "soc/rtc_cntl_reg.h"    // Library untuk kontrol daya (Brownout)
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h> 
#include <FirebaseESP32.h>
#include <ESP32QRCodeReader.h> 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- KONFIGURASI PIN ---
#define SDA_PIN 14
#define SCL_PIN 15
#define BUTTON_PIN 12 

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// --- KONFIGURASI DATABASE ---
#define API_KEY "3x5VUfpdd4sNNv0BAUvxw81KP0LjPlXxR3NeFQr9"
#define DATABASE_URL "https://empirise-79f29-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// --- OBJEK ---
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WiFiManager wm;

bool isSystemOn = true;       

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("OLED Error"); 
  }
  display.setRotation(2); 
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // 3. WiFi
  wm.setConfigPortalTimeout(180); 
  if (!wm.autoConnect("ABSENSI_CAM", "password123")) {
    ESP.restart();
  }
  tampilOled("WiFi Connected!", "Menunggu Kamera...");
  delay(500);

  reader.setup(); 
  reader.begin();
  
  delay(100); 
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(WHITE);

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = API_KEY;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);


  tampilOled("SIAP SCAN", "System Active");
  Serial.println("Sistem Siap! Arahkan QR Code.");
}

void loop() {

  if(digitalRead(BUTTON_PIN) == LOW) {

  }

  if (isSystemOn) {
    struct QRCodeData qrCodeData;
    
    if (reader.receiveQrCode(&qrCodeData, 100)) {
      if (qrCodeData.valid) {
        String qrCode = (const char *)qrCodeData.payload;
        
        Serial.println("QR DITEMUKAN: " + qrCode);

        tampilOled("QR Terbaca!", "Kirim Data...");
        
        prosesAbsen(qrCode);
        
        delay(2000); 
        tampilOled("SIAP SCAN", "Arahkan Kamera");
      }
    }
    delay(10); 
  }
}

void prosesAbsen(String qr) {
  String pathUser = "/users/" + qr;
  Serial.print("Kirim ke Firebase... ");
  
  // Kita coba ambil data
  if (Firebase.getString(fbdo, pathUser)) {
    Serial.println("BERHASIL!");
    String namaUser = fbdo.stringData();
    String pathLog = "/logs/" + String(millis()); 
    
    FirebaseJson json;
    json.set("nama", namaUser);
    json.set("qr", qr);
    json.set("waktu", "Auto"); 

    Firebase.setJSON(fbdo, pathLog, json);

    // Tampil Sukses di OLED
    display.clearDisplay();
    display.setCursor(0,0); display.setTextSize(2);
    display.println("HADIR!");
    display.setTextSize(1);
    display.println(namaUser);
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