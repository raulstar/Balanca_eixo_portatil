#include <Arduino.h>
#include "HX711.h"

#define DOUT_PIN  4
#define SCK_PIN   5

HX711 scale;
float calibration_factor = 420.0;

// Retorna o peso em gramas, ou -1 em caso de timeout
float lerPeso(int amostras = 10, int timeout_ms = 500) {
  if (!scale.wait_ready_timeout(timeout_ms)) {
    Serial.println("Timeout ao aguardar HX711.");
    return -1.0;
  }

  float peso = scale.get_units(amostras);

  // Filtro de valores negativos (ruído)
  if (peso < 0) peso = 0;

  return peso;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando balança...");

  scale.begin(DOUT_PIN, SCK_PIN);

  Serial.println("Zerando balança... Retire qualquer objeto!");
  delay(2000);

  scale.set_scale(calibration_factor);
  scale.tare();

  Serial.println("Pronto! Coloque o objeto.");
}

void loop() {
  float peso = lerPeso(10, 500);

  if (peso >= 0) {
    Serial.print("Peso: ");
    Serial.print(peso, 2);
   // Serial.println(" g");
  }

  delay(500);
}

float calcularMedia(float peso) {
  const int    NUM_AMOSTRAS     = 10;
  const float  LIMITE_INFERIOR  = 0.0;
  const float  LIMITE_SUPERIOR  = 10000.0;
  const int    TIMEOUT_MS       = 500;

  float amostras[NUM_AMOSTRAS];
  float soma          = 0.0;
  float media         = 0.0;
  int   validas       = 0;

  for (int i = 0; i < NUM_AMOSTRAS; i++) {

    if (!scale.wait_ready_timeout(TIMEOUT_MS)) {
      Serial.println("Timeout na amostra " + String(i));
      amostras[i] = peso; // usa o valor recebido como fallback
    } else {
      amostras[i] = scale.get_units(1);
    }

    // Descarta ruídos fora dos limites
    if (amostras[i] >= LIMITE_INFERIOR && amostras[i] <= LIMITE_SUPERIOR) {
      soma += amostras[i];
      validas++;
    }
  }

  if (validas == 0) {
    Serial.println("Nenhuma amostra válida. Retornando peso recebido.");
    return peso;
  }

  media = soma / (float)validas;
  return media;
}