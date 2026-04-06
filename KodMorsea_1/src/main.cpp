#include <Arduino.h>

#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // scl - 7, sda - 6 piny i2c

#define BATTERY_PIN 2 // Pin bateri
uint32_t battery = 0; // Poziom baterii
uint32_t batteryLevel = 100; // Poziom baterii (0-100%)

#define BUZZER_PIN 20 // GPIO2 (D0)
#define BUZZER_CH 0 // Kanał LEDC (0-7)
uint8_t volume = 20; // 50% wypełnienia, 50% głośności
uint32_t freq = 1500; // Standardowa nuta A4

#define B_LEFT_PIN 5 // Pin lewego przycisku
#define B_RIGHT_PIN 21 // Pin prawego przycisku
bool left = 0;
bool right = 0;

uint8_t n = 5; // Długość kodu Morse'a
uint8_t m = 36; // Ilość znaków do dekodowania
uint8_t n_c = 0; //Położenie znacznika
uint8_t n_d = 0; // Położenie dekodera
uint8_t letterPlacment = 9; // Położenie litery na ekranie

uint32_t code [5] = {0, 0, 0, 0, 0};
char decode [36] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
uint32_t key [36] [5] = { // 1 -> dit, 2 -> dah, 0 -> przerwa
  {1, 2, 0, 0, 0}, // A
  {2, 1, 1, 1, 0}, // B
  {2, 1, 2, 1, 0}, // C
  {2, 1, 1, 0, 0}, // D
  {1, 0, 0, 0, 0}, // E
  {1, 1, 2, 1, 0}, // F
  {2, 2, 1, 0, 0}, // G
  {1, 1, 1, 1, 0}, // H
  {1, 1, 0, 0, 0}, // I
  {1, 2, 2, 2, 0}, // J
  {2, 1, 2, 0, 0}, // K
  {1, 2, 1, 1, 0}, // L
  {2, 2, 0, 0, 0}, // M
  {2, 1, 0, 0, 0}, // N
  {2, 2, 2, 0, 0}, // O
  {1, 2, 2, 1, 0}, // P
  {2, 2, 1, 2, 0}, // Q
  {1, 2, 1, 0, 0}, // R
  {1, 1, 1, 0, 0}, // S
  {2, 0, 0, 0, 0}, // T
  {1, 1, 2, 0, 0}, // U
  {1, 1, 1, 2, 0}, // V
  {1, 2, 2, 0, 0}, // W
  {2, 1, 1, 2, 0}, // X
  {2, 1, 2, 2, 0}, // Y
  {2, 2, 1, 1, 0}, // Z
  {2, 2, 2, 2, 2}, // 0
  {1, 2, 2, 2, 2}, // 1
  {1, 1, 2, 2, 2}, // 2
  {1, 1, 1, 2, 2}, // 3
  {1, 1, 1, 1, 2}, // 4
  {1, 1, 1, 1, 1}, // 5
  {2, 1, 1, 1, 1}, // 6
  {2, 2, 1, 1, 1}, // 7
  {2, 2, 2, 1, 1}, // 8
  {2, 2, 2, 2, 1}, // 9
};
char messenge [10];

uint32_t dit_time = 120; // Czas trwanie podstawowej jednostki w ms (80 ms)
uint32_t dah_time = dit_time * 3; // Czas trwania kreski
uint32_t inLetter = dit_time;
uint32_t nextLetter = dit_time * 3;
uint32_t nextWord = dit_time * 7;

uint32_t t = 0; // Pomoc w czasie
uint32_t b = 0; // Pomoc w czasie
bool space = 1; // Pomoc w rozdzielaniu słów

void checkBatteryLevel() {
  battery = analogRead(BATTERY_PIN);
  batteryLevel = map(battery, 2300, 2835, 0, 100);
  if (batteryLevel > 100) {
    batteryLevel = 100;
  }
  display.fillRect(95, 0, 35, 18, SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(100, 6);
  display.print(batteryLevel);
  display.print("%");
  display.setTextSize(2);
  display.setCursor(2, 48);
  display.display();
}
void checkButtons(){
  if (digitalRead(B_LEFT_PIN) == HIGH) {
    left = 1;
  }
  if (digitalRead(B_RIGHT_PIN) == HIGH) {
    right = 1;
  }
}

void playTone(uint32_t freq, uint32_t duration_ms) {
  if (freq == 0) {
    ledcWrite(BUZZER_CH, 0); // Cisza
  } else {
    ledcWriteTone(BUZZER_CH, freq);
    ledcWrite(BUZZER_CH, volume); // 50% duty cycle = najgłośniej
  }
  delay(duration_ms);
}
void silence (uint32_t duration_ms) {
  ledcWrite(BUZZER_CH, 0);
  delay(duration_ms);
}

void screenMain() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  display.drawLine(93, 0, 93, 18, SSD1306_WHITE);
  display.drawLine(0, 18, 128, 18, SSD1306_WHITE);
  display.drawLine(0, 44, 128, 44, SSD1306_WHITE);

  display.setCursor(2, 6);
  display.print("dit_time: ");
  display.print(dit_time);
  display.print("ms");

  display.setCursor(100, 6);
  display.print(batteryLevel);
  display.print("%");

  display.setTextSize(2);
  display.setCursor(2, 24);
  display.print("");

  display.setCursor(2, 48); // Kod Morse'a
  display.print("");

  
  display.display();
}
void codeWriting() {
  if (n_c >= n) {
    n_c = 0;
    display.fillRect(0, 48, 128, 16, SSD1306_BLACK);
    display.setCursor(2, 48);
    display.display();
    for (uint8_t i = 0; i < n; i++) {
      code[i] = 0;
    }
  }
  if (left == 1) {
    display.setCursor(2 + n_c * 12, 48);
    display.print(".");
    display.display();
    code[n_c] = 1;
    n_c = n_c + 1;
  }
  if (right == 1) {
    display.setCursor(2 + n_c * 12, 48);
    display.print("-");
    display.display();
    code[n_c] = 2;
    n_c = n_c + 1;
  }
}
void decodeWriting() {
  for (uint8_t i = 0; i < m; i++) {
    bool match = true;
    for (uint8_t j = 0; j < n; j++) {
      if (code[j] != key[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      for (uint8_t k = 0; k < 10; k++) {
        messenge[k - 1] = messenge[k];
      }
      messenge[9] = decode[i];
      display.fillRect(0, 24, 128, 18, SSD1306_BLACK);
      for (uint8_t k = 0; k < 10; k++) { 
        display.setCursor(2 + k * 12, 24);
        display.print(messenge[k]);
      }
      break;
    }
  }
  display.display();
  display.setCursor(2, 48);
}


void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  screenMain();

  ledcSetup(BUZZER_CH, 2000, 8); // Kanał, max częstotliwość, rozdzielczość (8 bitów)
  ledcAttachPin(BUZZER_PIN, BUZZER_CH);
  ledcWrite(BUZZER_CH, 0);

  pinMode(B_LEFT_PIN, INPUT_PULLDOWN);
  pinMode(B_RIGHT_PIN, INPUT_PULLDOWN);

  checkBatteryLevel();
}

void loop() {
  while(1){
    if (millis() - b > 1000) {
      checkBatteryLevel();
      b = millis();
    }
    checkButtons();

    if (left == 1 || right == 1) {
      if (left == 1) {
        codeWriting();
        playTone(freq, dit_time);
        silence(dit_time); 
        left = 0;
      }
      if (right == 1) {
        codeWriting();
        playTone(freq, dah_time);
        silence(dit_time); 
        right = 0;
      }
      t = millis();
      space = 0;
    }
    if (millis() - t > nextLetter) {
      decodeWriting();
      n_c = n; 
      codeWriting();
    }
    if (millis() - t > nextWord && space == 0) {
      t = millis();
      for (uint8_t k = 0; k < 10; k++) {
        messenge[k - 1] = messenge[k];
      }
      messenge[9] = ' ';
      space = 1;
    }
  }
}




