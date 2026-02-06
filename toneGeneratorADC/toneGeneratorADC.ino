/*
 * Project: Low Pass Filter 4kHz - ESP8266 Version
 * Hardware: Wemos D1 R1 (Based on ESP8266)
 * Input Pin: A0 (Satu-satunya pin ADC)
 */

const int sensorPin = A0;   // Pin Analog ESP8266
int midPoint = 512;         // Estimasi nilai tengah (Range 0-1023)
bool lastState = false;
unsigned long lastTime = 0;
float frequency = 0;

void setup() {
  // Baudrate tinggi untuk Serial Plotter yang mulus
  Serial.begin(115200);
  
  // PENTING: ESP8266 tidak support 'analogReadResolution'.
  // Dia fix di 10-bit (0 - 1023). Jadi tidak perlu setting resolusi.
  
  // --- PROSES KALIBRASI OTOMATIS ---
  Serial.println("Kalibrasi dimulai...");
  long sum = 0;
  // Ambil 100 sampel awal untuk mencari titik 0 (Virtual Ground)
  for (int i = 0; i < 100; i++) {
    sum += analogRead(sensorPin);
    delay(2);
  }
  midPoint = sum / 100;
  
  Serial.print("Kalibrasi Selesai. Mid Point di: ");
  Serial.println(midPoint);
  delay(1000); 
}

void loop() {
  // 1. Baca data analog (0 - 1023)
  int val = analogRead(sensorPin);

  // 2. Deteksi Frekuensi (Zero Crossing)
  // Kita cek apakah sinyal menyeberangi garis tengah
  bool currentState = val > midPoint;

  // Jika sinyal naik melewati batas tengah (Rising Edge)
  if (currentState && !lastState) {
    unsigned long now = micros();
    if (lastTime > 0) {
      // Hitung selisih waktu antar gelombang
      unsigned long diff = now - lastTime;
      // Rumus: 1 juta mikrodetik / selisih waktu = Frekuensi (Hz)
      frequency = 1000000.0 / diff;
    }
    lastTime = now;
  }
  lastState = currentState;

  // 3. Output ke Serial Plotter
  // Format: Label:Nilai
  
  Serial.print("Sinyal:");
  Serial.print(val);
  
  Serial.print(",MidPoint:");
  Serial.print(midPoint);
  
  Serial.print(",Frekuensi:");
  Serial.println(frequency); // Harusnya muncul angka sekitar 4000
}