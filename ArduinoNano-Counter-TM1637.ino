#include <TM1637Display.h>

// Definovanie pinu pre CLK a DIO
#define CLK 2  // D2 na Arduino Nano
#define DIO 3  // D3 na Arduino Nano

// Vytvorenie inštancie pre displej
TM1637Display display(CLK, DIO);

void setup() {
  display.setBrightness(0x0f);  // Nastavenie maximálneho jasu displeja
}

void loop() {
  // Cyklus pre zobrazenie čísiel od 0001 po 9999
  for (int i = 1; i <= 9999; i++) {
    display.showNumberDec(i, true, 4, 0);  // Zobrazenie čísla na 4 miestach
    delay(300);  // 0,3 sekundy oneskorenie
  }
}
