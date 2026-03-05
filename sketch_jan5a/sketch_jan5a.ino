#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID           "TMPL6UB7m3KJj"
#define BLYNK_TEMPLATE_NAME         "dwyn"
#define BLYNK_AUTH_TOKEN            "D3zJPi6rfmbzbmQu_jNYtPEwCcV9Av6f"

#define relay1 19
#define relay2 18
#define ldrpin 34

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h> 
#include <LCD_I2C.h>
#include <DHT11.h>

char ssid[] = "Kelompok 10";
char pass[] = "12345678";

LCD_I2C lcd(0x27, 16, 2);
DHT11 dht11(2);

void setup()
{
  Serial.begin(9600);
  pinMode(ldrpin, INPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);

  lcd.begin();       
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Demo Project");
  lcd.setCursor(5,1);
  lcd.print("Kelompok 10");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  delay(2000);
}

void loop()
{
  Blynk.run();

  int suhu = dht11.readTemperature();
  int humid = dht11.readHumidity();
  int data = analogRead(ldrpin)*0.6;

  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Suhu      : ");
  lcd.print(suhu);
  lcd.println(" °C");

  lcd.setCursor(0,1);
  lcd.print("Kelembapan: ");
  lcd.print(humid);
  lcd.println("%");
  delay(2000);
  
  if (humid > 250 && suhu < 30){
    digitalWrite(relay1,LOW);
  } 
  else {
    digitalWrite(relay1,HIGH);
  }

  Serial.println(data);
  if(data <= 50){
    digitalWrite(relay2, LOW);
  }
  else {
    digitalWrite(relay2, HIGH);
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("cahaya  : ");
  lcd.print(data);
  lcd.println(" lux");
  delay(2000);
}

void sendSensor()
{
  int suhu = dht11.readTemperature();
  int kelembapan = dht11.readHumidity();

  if (isnan(kelembapan) || isnan(suhu)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Blynk.virtualWrite(V0, kelembapan);
  Blynk.virtualWrite(V1, suhu);
}