#include <Arduino.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include "PaginaHTML.h"
#define DOUT_PIN 4
#define SCK_PIN 5
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HX711 scale;
WebServer server(80);

const char *ssid = "REVLO";
const char *password = "Revlo!2024";
float calibration_factor = 420.0;
float pesoAtual = 0;
float mediaFinal = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float lerPeso(int amostras, int timeout_ms);
float calcularMedia(float peso);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// -------------------------------------------------------
// Declare aqui sua instância do HX711 normalmente
// HX711 scale;
// -------------------------------------------------------

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
  delay(8000);

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

void handleRoot()
{
  server.send_P(200, "text/html", pagina_html);
}

void handleDados()
{
  String json = "{";
  json += "\"pesoAtual\":" + String(pesoAtual, 4) + ",";
  json += "\"calibration_factor\":" + String(calibration_factor, 4);
  json += "}";
  server.send(200, "application/json", json);
}

void handleCalibrar() {
  if (!server.hasArg("peso")) {
    server.send(400, "text/plain", "Parametro 'peso' ausente.");
    return;
  }
  float pesoConhecido = server.arg("peso").toFloat();
  if (pesoConhecido <= 0) {
    server.send(400, "text/plain", "Peso invalido.");
    return;
  }
  realizarCalibracao(pesoConhecido);
  server.send(200, "text/plain", "Calibracao realizada com " + String(pesoConhecido, 2) + " kg.");
}
void handleZero()
{
   scale.tare();
  Serial.println("Tara realizada.");
  server.send(200, "text/plain", "Zero (tara) aplicado com sucesso.");
}

void handleNotFound()
{
  server.send(404, "text/plain", "Pagina nao encontrada.");
}

void setup()
{
  Serial.begin(115200);
  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  WiFi.begin(ssid, password);
  Serial.print("Conectando");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/dados", handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/zero", handleZero);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop()
{
  server.handleClient();
  pesoAtual = scale.get_units(10);
  mediaFinal = calcularMedia(pesoAtual);

  Serial.println("");
  Serial.print("peso: ");
  Serial.print(pesoAtual, 2);
  Serial.println(" g");

  Serial.print(pesoAtual / 1000, 1);
  Serial.println(" kg");

  delay(400);
}

