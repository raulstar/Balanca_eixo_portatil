#include <Arduino.h>
#include "HX711.h"

#define DOUT_PIN 4
#define SCK_PIN 5
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX711 scale;
float calibration_factor = 420.0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float lerPeso(int amostras, int timeout_ms);
float calcularMedia(float peso);
void realizarCalibracao(float pesoConhecido);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando balança...");
  scale.begin(DOUT_PIN, SCK_PIN);

  Serial.println("Zerando balança... Retire qualquer objeto!");
  delay(2000);

  scale.set_scale(calibration_factor);
  scale.tare();
  //realizarCalibracao(2000.0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  float pesoAtual = scale.get_units(10);
  float mediaFinal = calcularMedia(pesoAtual);

  Serial.println("");
  Serial.print("peso: ");
  Serial.print(mediaFinal, 2);
  Serial.println(" g");

  Serial.print(mediaFinal / 1000, 1);
  Serial.println(" kg");

  delay(500);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retorna o peso em gramas, ou -1 em caso de timeout
float lerPeso(int amostras = 10, int timeout_ms = 500)
{
  if (!scale.wait_ready_timeout(timeout_ms))
  {
    Serial.println("Timeout ao aguardar HX711.");
    return -1.0;
  }

  float peso = scale.get_units(amostras);
  // Filtro de valores negativos (ruído)
  if (peso < 0)
    peso = 0;

  return peso;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float calcularMedia(float peso)
{
  const int NUM_AMOSTRAS = 10;
  const float LIMITE_INFERIOR = 0.0;
  const float LIMITE_SUPERIOR = 10000.0;
  const int TIMEOUT_MS = 500;

  float amostras[NUM_AMOSTRAS];
  float soma = 0.0;
  float media = 0.0;
  int validas = 0;

  for (int i = 0; i < NUM_AMOSTRAS; i++)
  {

    if (!scale.wait_ready_timeout(TIMEOUT_MS))
    {
      Serial.println("Timeout na amostra " + String(i));
      amostras[i] = peso; // usa o valor recebido como fallback
    }
    else
    {
      amostras[i] = scale.get_units(1);
    }

    // Descarta ruídos fora dos limites
    if (amostras[i] >= LIMITE_INFERIOR && amostras[i] <= LIMITE_SUPERIOR)
    {
      soma += amostras[i];
      validas++;
    }
  }

  if (validas == 0)
  {
    Serial.println("Nenhuma amostra válida. Retornando peso recebido.");
    return peso;
  }

  media = soma / (float)validas;
  return media;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void realizarCalibracao(float pesoConhecido)
{
  Serial.println("\n--- INICIANDO CALIBRAÇÃO ---");

  // Passo 1: Zera a balança (Tare)
  scale.set_scale(); // Reseta o fator para 1.0
  scale.tare();      // Define o ponto zero atual
  Serial.println("Balança zerada.");

  Serial.print("2. Coloque o peso de ");
  Serial.print(pesoConhecido);
  Serial.println("g sobre a balança.");
  Serial.println("Aguardando 10 segundos para estabilização...");
  delay(10000);

  // Passo 2: Lê o valor bruto (raw) com o peso em cima
  // get_units(10) aqui retornará a média das leituras brutas menos o tare
  float leituraBruta = scale.get_units(10);

  // Passo 3: Calcula o novo fator (Fator = Leitura / Peso Real)
  calibration_factor = leituraBruta / pesoConhecido;

  // Aplica o novo fator
  scale.set_scale(calibration_factor);

  Serial.println("--- CALIBRAÇÃO CONCLUÍDA ---");
  Serial.print("Novo Fator de Calibração: ");
  Serial.println(calibration_factor);
  Serial.println("Anote este valor para usar no código permanentemente.\n");
}