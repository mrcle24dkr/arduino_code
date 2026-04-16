#include <SoftwareSerial.h>

// Coba konfigurasi ini dulu
#define RX_PIN D5  // Sambung ke Kabel PUTIH (atau HIJAU)
#define TX_PIN D6  // Sambung ke Kabel HIJAU (atau PUTIH)

SoftwareSerial ScannerSerial(RX_PIN, TX_PIN);

void setup() {
  Serial.begin(115200); // Untuk Laptop
  ScannerSerial.begin(9600); // Coba ganti 9600 atau 115200 disini
  
  Serial.println("=== MODE TES KONEKSI ===");
  Serial.println("Coba scan sesuatu...");
}

void loop() {
  if (ScannerSerial.available()) {
    // Baca karakter satu per satu biar ketahuan kalau ada data masuk
    char c = ScannerSerial.read();
    
    Serial.print("Data Masuk: ");
    Serial.println(c); 
    
    // Atau baca string langsung
    // String s = ScannerSerial.readString();
    // Serial.println(s);
  }
}