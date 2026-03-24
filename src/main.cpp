#include <Arduino.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

/////////////////////////////////////////////////////////////////////////////
//  PINOS
#define DOUT_PIN 4
#define SCK_PIN 5

#define NEXTION_SERIAL Serial2
#define NEXTION_BAUD 9600

/////////////////////////////////////////////////////////////////////////////
//  OBJETOS E VARIÁVEIS GLOBAIS
HX711 scale;
WebServer server(80);

const char *ssid = "REVLO";
const char *password = "Revlo!2024";

float calibration_factor = 88.0706;
float pesoAtual = 0.0;
bool zero = false;

unsigned long ultimaAtualizacaoTela = 0;
const unsigned long INTERVALO_TELA = 200;

/////////////////////////////////////////////////////////////////////////////
//  PROTÓTIPOS
void nextionCmd(const String &cmd);
void atualizarPesoNaTela();
void realizarCalibracao(float pesoConhecido);
void lerNextion();
void handleRoot();
void handleDados();
void handleCalibrar();
void handleZero();
bool calculoDeZero(float peso);
float filtro(float amostra);

/////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(115200);

  NEXTION_SERIAL.begin(NEXTION_BAUD, SERIAL_8N1, 16, 17);
  delay(500);
  nextionCmd("page 0");
  nextionCmd("tPeso.txt=\"Iniciando...\"");

  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  // ================= WIFI COM TENTATIVAS =================
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");

  int tentativas = 0;
  const int maxTentativas = 10;

  while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas)
  {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Conectado!");
    Serial.println("IP: " + WiFi.localIP().toString());

    if (MDNS.begin("balanca"))
    {
      Serial.println("MDNS iniciado: http://balanca.local");
      MDNS.addService("http", "tcp", 80);
    }
    else
    {
      Serial.println("Erro ao iniciar MDNS");
    }
  }
  else
  {
    Serial.println("\nFalha ao conectar no WiFi.");
    // Opcional: reiniciar automaticamente
    // ESP.restart();
  }
  // ======================================================

  server.on("/", handleRoot);
  server.on("/dados", handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/zero", handleZero);
  server.onNotFound([]()
                    { server.send(404, "text/plain", "Não encontrado."); });

  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

/////////////////////////////////////////////////////////////////////////////
//  LOOP
void loop()
{
  server.handleClient();
  lerNextion();

  if (scale.is_ready())
  {
    float medida = scale.get_units(1);
    zero = calculoDeZero(medida);
    pesoAtual = filtro(medida);
  }

  unsigned long agora = millis();
  if (agora - ultimaAtualizacaoTela >= INTERVALO_TELA)
  {
    ultimaAtualizacaoTela = agora;
    atualizarPesoNaTela();
  }

  static unsigned long ultimoPrint = 0;
  if (millis() - ultimoPrint >= 500)
  {
    ultimoPrint = millis();
    Serial.printf("Peso: %.2f kg \n", pesoAtual);
  }
}

/////////////////////////////////////////////////////////////////////////////
void nextionCmd(const String &cmd)
{
  NEXTION_SERIAL.print(cmd);
  NEXTION_SERIAL.write(0xFF);
  NEXTION_SERIAL.write(0xFF);
  NEXTION_SERIAL.write(0xFF);
}

/////////////////////////////////////////////////////////////////////////////
void atualizarPesoNaTela()
{
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.2f Kg", pesoAtual);
  nextionCmd(String("tPeso.txt=\"") + buffer + "\"");
}

/////////////////////////////////////////////////////////////////////////////
void realizarCalibracao(float pesoConhecido)
{
  scale.set_scale();
  scale.tare();

  nextionCmd("tPeso.txt=\"Aguarde...\"");
  delay(5000);

  float leituraBruta = scale.get_units(20);
  calibration_factor = leituraBruta / pesoConhecido;
  scale.set_scale(calibration_factor);

  nextionCmd("tPeso.txt=\"Calibrado!\"");
  delay(1000);
}

/////////////////////////////////////////////////////////////////////////////
void lerNextion()
{
  if (!NEXTION_SERIAL.available())
    return;

  String entrada = NEXTION_SERIAL.readStringUntil('\xFF');
  entrada.trim();

  if (entrada == "CALIB")
    realizarCalibracao(100.0);
  else if (entrada.startsWith("CALIB:"))
    realizarCalibracao(entrada.substring(6).toFloat());
  else if (entrada == "ZERO")
    handleZero();
}

/////////////////////////////////////////////////////////////////////////////
void handleRoot()
{
  server.send_P(200, "text/html", pagina_html);
}

/////////////////////////////////////////////////////////////////////////////
void handleDados()
{
  String json = "{";
  json += "\"pesoAtual\":" + String(max(0.0f, pesoAtual), 4) + ",";
  json += "\"calibration_factor\":" + String(calibration_factor, 4);
  json += "}";
  server.send(200, "application/json", json);
}

/////////////////////////////////////////////////////////////////////////////
void handleCalibrar()
{
  if (!server.hasArg("peso"))
  {
    server.send(400, "text/plain", "Erro: Informe o peso.");
    return;
  }

  float pesoConhecido = server.arg("peso").toFloat();
  server.send(200, "text/plain", "Calibrando...");
  realizarCalibracao(pesoConhecido);
}

/////////////////////////////////////////////////////////////////////////////
void handleZero()
{
  scale.tare();
  pesoAtual = 0.0;

  if (server.client())
    server.send(200, "text/plain", "Zerado!");

  nextionCmd("tPeso.txt=\"Zerado!\"");
  delay(800);
}

/////////////////////////////////////////////////////////////////////////////
bool calculoDeZero(float peso)
{
  return (peso >= 1.0);
}

/////////////////////////////////////////////////////////////////////////////
float filtro(float amostra)
{
  const int N = 10;
  const float FAIXA_MODA = 0.5;

  static float amostras[N];
  static int count = 0;
  static float ultimoValido = 0;

  if (!zero)
  {
    count = 0;
    ultimoValido = 0;
    return 0;
  }

  amostras[count++] = amostra;

  if (count < N)
    return ultimoValido;

  float soma = 0;
  for (int i = 0; i < N; i++)
    soma += amostras[i];
  float media = soma / N;

  int melhorContagem = 0;
  float melhorCentro = amostras[0];

  for (int i = 0; i < N; i++)
  {
    int contagem = 0;
    for (int j = 0; j < N; j++)
      if (fabs(amostras[i] - amostras[j]) <= FAIXA_MODA)
        contagem++;

    if (contagem > melhorContagem)
    {
      melhorContagem = contagem;
      melhorCentro = amostras[i];
    }
  }

  float somaFiltrada = 0;
  int countFiltrado = 0;

  for (int i = 0; i < N; i++)
  {
    if (fabs(amostras[i] - melhorCentro) <= FAIXA_MODA)
    {
      somaFiltrada += amostras[i];
      countFiltrado++;
    }
  }

  ultimoValido = (countFiltrado > 0) ? (somaFiltrada / countFiltrado) : media;

  count = 0;
  return ultimoValido;
}