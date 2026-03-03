#include <Arduino.h>
#include <EEPROM.h>
#include "medida.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ─── Pinos ────────────────────────────────────────────────────────
#define DOUT_PIN  4
#define SCK_PIN   5
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ─── Fator de calibração ──────────────────────────────────────────
// Ajuste este valor com um peso conhecido para calibrar sua balança
float fatorDeEscala = 2280.0;

// ─── Instância da classe ──────────────────────────────────────────
Medida medida(DOUT_PIN, SCK_PIN);

// ─── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  medida.begin(fatorDeEscala);
}

// ─── Loop ─────────────────────────────────────────────────────────
void loop() {
  if (medida.isReady()) {

    // Leitura digital crua do ADC (sem escala, sem tare)
    long  digitalRaw = medida.readDigitalRaw();

    // Leitura em gramas sem filtro (com escala e tare aplicados)
    float rawGrams   = medida.readRawGrams();

    // Leitura com filtro aplicado
    float filtered   = medida.readFiltered();

    Serial.printf("│ %15ld │ %13.2f g │ %13.2f g │\n",
                  digitalRaw, rawGrams, filtered);
  } else {
    Serial.println("HX711 não está pronto.");
  }

  delay(500);
}