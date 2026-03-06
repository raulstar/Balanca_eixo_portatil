#include <Arduino.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

#define DOUT_PIN 4
#define SCK_PIN  5

HX711     scale;
WebServer server(80);

const char *ssid     = "REVLO";
const char *password = "Revlo!2024";

float calibration_factor = 88.0706;
float pesoAtual          = 0;
bool  zero               = false;

// -------------------------------------------------------
// CALCULO DE ZERO
// -------------------------------------------------------
bool calculoDeZero(float peso)
{
  const float PESO_MINIMO = 1.0; // gramas
  return (peso >= PESO_MINIMO);
}

// -------------------------------------------------------
// FILTRO — média da moda, não bloqueante
// -------------------------------------------------------
float filtro(float amostra)
{
  const int   N          = 10;
  const float FAIXA_MODA = 0.5; // gramas

  static float amostras[N];
  static int   count          = 0;
  static float ultimoValido   = 0;

  // Se não há peso válido, reseta e devolve 0
  if (!zero)
  {
    count       = 0;
    ultimoValido = 0;
    return 0;
  }

  // Acumula amostras
  amostras[count++] = amostra;

  // Ainda coletando — devolve o último resultado calculado
  if (count < N)
    return ultimoValido;

  // -------------------------------------------------------
  // Buffer cheio — processa
  // -------------------------------------------------------

  // 1. Média geral
  float soma = 0;
  for (int i = 0; i < N; i++) soma += amostras[i];
  float media = soma / N;

  // 2. Desvio padrão
  float somaQuad = 0;
  for (int i = 0; i < N; i++)
    somaQuad += (amostras[i] - media) * (amostras[i] - media);
  float desvioPadrao = sqrt(somaQuad / N);

  Serial.printf("[filtro] Media: %.4f | Desvio Padrao: %.4f\n", media, desvioPadrao);

  // 3. Moda — grupo mais frequente dentro da tolerância
  int   melhorContagem = 0;
  float melhorCentro   = amostras[0];

  for (int i = 0; i < N; i++)
  {
    int contagem = 0;
    for (int j = 0; j < N; j++)
      if (fabs(amostras[i] - amostras[j]) <= FAIXA_MODA)
        contagem++;

    if (contagem > melhorContagem)
    {
      melhorContagem = contagem;
      melhorCentro   = amostras[i];
    }
  }

  Serial.printf("[filtro] Moda: %.4f | Grupo: %d amostras\n", melhorCentro, melhorContagem);

  // 4. Média apenas das amostras dentro da moda
  float somaFiltrada  = 0;
  int   countFiltrado = 0;

  for (int i = 0; i < N; i++)
  {
    if (fabs(amostras[i] - melhorCentro) <= FAIXA_MODA)
    {
      somaFiltrada += amostras[i];
      countFiltrado++;
    }
    else
    {
      //Serial.printf("[filtro] Descartado: %.4f\n", amostras[i]);
    }
  }

  ultimoValido = (countFiltrado > 0) ? (somaFiltrada / countFiltrado) : media;

  Serial.printf("[filtro] Resultado: %.4f (usou %d/%d amostras)\n",
                ultimoValido, countFiltrado, N);

  count = 0; // reseta buffer
  return ultimoValido;
}

// -------------------------------------------------------
// CALIBRAÇÃO
// -------------------------------------------------------
void realizarCalibracao(float pesoConhecido)
{
  Serial.println("\n--- INICIANDO CALIBRAÇÃO ---");
  scale.set_scale();
  scale.tare();
  Serial.printf("Coloque %.2fg. Aguardando 5s...\n", pesoConhecido);
  delay(5000);
  float leituraBruta = scale.get_units(20);
  calibration_factor = leituraBruta / pesoConhecido;
  scale.set_scale(calibration_factor);
  Serial.printf("--- CALIBRAÇÃO CONCLUÍDA --- Fator: %.6f\n", calibration_factor);
}

// -------------------------------------------------------
// HANDLERS
// -------------------------------------------------------
void handleRoot()
{
  server.send_P(200, "text/html", pagina_html);
}

void handleDados()
{
  String pesoStr = zero ? String(pesoAtual, 4) : "null";
  String json    = "{";
  json += "\"pesoAtual\":"          + pesoStr + ",";
  json += "\"calibration_factor\":" + String(calibration_factor, 4);
  json += "}";
  server.send(200, "application/json", json);
}

void handleCalibrar()
{
  if (server.hasArg("peso"))
  {
    float pesoConhecido = server.arg("peso").toFloat();
    if (pesoConhecido > 0)
    {
      realizarCalibracao(pesoConhecido);
      server.send(200, "text/plain",
                  "Calibrado com " + String(pesoConhecido) + "g. Fator: "
                  + String(calibration_factor, 4));
      return;
    }
  }
  server.send(400, "text/plain", "Peso invalido");
}

void handleZero()
{
  scale.tare();
  pesoAtual = 0;
  zero      = false;
  Serial.println("Tara realizada via Web.");
  server.send(200, "text/plain", "Balança zerada com sucesso.");
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado! IP: " + WiFi.localIP().toString());

  if (MDNS.begin("balanca"))
    MDNS.addService("http", "tcp", 80);

  server.on("/",         handleRoot);
  server.on("/dados",    handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/zero",     handleZero);
  server.onNotFound([]() { server.send(404, "text/plain", "Nao encontrado"); });

  server.begin();
}

// -------------------------------------------------------
// LOOP — ordem correta das operações
// -------------------------------------------------------
void loop()
{
  server.handleClient();

  if (scale.is_ready())
  {
    float medida = scale.get_units(1);  // leitura bruta do HX711
    zero         = calculoDeZero(medida); // avalia a leitura ATUAL, não a anterior
    pesoAtual    = filtro(medida);        // filtra com zero já atualizado
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500)
  {
    Serial.printf("Peso: %.4f g | Zero: %s\n", pesoAtual, zero ? "true" : "false");
    lastPrint = millis();
  }
}