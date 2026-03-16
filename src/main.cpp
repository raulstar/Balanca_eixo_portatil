#include <Arduino.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

// ─────────────────────────────────────────
//  PINOS
// ─────────────────────────────────────────
#define DOUT_PIN 4
#define SCK_PIN  5

#define NEXTION_SERIAL Serial2
#define NEXTION_BAUD   9600

// ─────────────────────────────────────────
//  OBJETOS E VARIÁVEIS GLOBAIS
// ─────────────────────────────────────────
HX711     scale;
WebServer server(80);

const char *ssid     = "REVLO";
const char *password = "Revlo!2024";

float calibration_factor = -409.8214;
float pesoAtual          = 0.0;

unsigned long ultimaAtualizacaoTela = 0;
const unsigned long INTERVALO_TELA  = 200;

// ─────────────────────────────────────────
//  PROTÓTIPOS
// ─────────────────────────────────────────
void nextionCmd(const String &cmd);
void atualizarPesoNaTela();
void realizarCalibracao(float pesoConhecido);
void lerNextion();
void handleRoot();
void handleDados();
void handleCalibrar();
void handleZero();

// ─────────────────────────────────────────
//  NEXTION — AUXILIARES
// ─────────────────────────────────────────
void nextionCmd(const String &cmd)
{
  NEXTION_SERIAL.print(cmd);
  NEXTION_SERIAL.write(0xFF);
  NEXTION_SERIAL.write(0xFF);
  NEXTION_SERIAL.write(0xFF);
}

void atualizarPesoNaTela()
{
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.2f g", pesoAtual);
  nextionCmd(String("tPeso.txt=\"") + buffer + "\"");
}

// ─────────────────────────────────────────
//  CALIBRAÇÃO
// ─────────────────────────────────────────
void realizarCalibracao(float pesoConhecido)
{
  Serial.println("\n--- INICIANDO CALIBRAÇÃO ---");
  Serial.printf("Peso de referência: %.2f g\n", pesoConhecido);

  scale.set_scale();
  scale.tare();

  nextionCmd("tPeso.txt=\"Aguarde...\"");
  Serial.println("Aguardando 5s para estabilizar...");
  delay(5000);

  float leituraBruta = scale.get_units(20);
  calibration_factor = leituraBruta / pesoConhecido;
  scale.set_scale(calibration_factor);

  Serial.println("--- CALIBRAÇÃO CONCLUÍDA ---");
  Serial.printf("Novo fator: %.4f\n", calibration_factor);

  nextionCmd("tPeso.txt=\"Calibrado!\"");
  delay(1000);
}

// ─────────────────────────────────────────
//  NEXTION — LÊ EVENTOS DE TOQUE
//
//  No Nextion Editor configure os botões:
//    bCalib → Touch Release Event:
//      prints "CALIB:",0
//      prints tInput.txt,0
//      printh FF FF FF
//
//    bZero → Touch Release Event:
//      printh 5A 45 52 4F FF FF FF
// ─────────────────────────────────────────
void lerNextion()
{
  if (!NEXTION_SERIAL.available()) return;

  String entrada = NEXTION_SERIAL.readStringUntil('\xFF');
  entrada.trim();

  if (entrada.length() == 0) return;

  // Formato esperado do botão Calibrar: "CALIB:500.00"
  if (entrada.startsWith("CALIB:"))
  {
    String valorStr = entrada.substring(6);
    float peso = valorStr.toFloat();

    if (peso > 0.0)
    {
      Serial.printf("[Nextion] Botão Calibrar pressionado — peso: %.2f g\n", peso);
      realizarCalibracao(peso);
    }
    else
    {
      Serial.println("[Nextion] Peso inválido recebido para calibração.");
      nextionCmd("tPeso.txt=\"Peso inv.\"");
      delay(800);
    }
  }
  else if (entrada == "ZERO")
  {
    Serial.println("[Nextion] Botão Zero pressionado.");
    handleZero();
  }
}

// ─────────────────────────────────────────
//  HANDLERS DO SERVIDOR WEB
// ─────────────────────────────────────────
void handleRoot()
{
  server.send_P(200, "text/html", pagina_html);
}

void handleDados()
{
  String json = "{";
  json += "\"pesoAtual\":"          + String(max(0.0f, pesoAtual), 4) + ",";
  json += "\"calibration_factor\":" + String(calibration_factor, 4);
  json += "}";
  server.send(200, "application/json", json);
}

void handleCalibrar()
{
  if (!server.hasArg("peso"))
  {
    server.send(400, "text/plain", "Parâmetro 'peso' ausente.");
    return;
  }

  float pesoConhecido = server.arg("peso").toFloat();

  if (pesoConhecido <= 0.0)
  {
    server.send(400, "text/plain", "Peso inválido.");
    return;
  }

  realizarCalibracao(pesoConhecido);
  server.send(200, "text/plain",
              "Calibrado com " + String(pesoConhecido, 2) + " g. Fator: "
              + String(calibration_factor, 4));
}

void handleZero()
{
  scale.tare();
  pesoAtual = 0.0;
  Serial.println("Tara realizada.");
  nextionCmd("tPeso.txt=\"Zerado!\"");
  delay(800);

  // Responde ao servidor Web apenas se a chamada vier via HTTP
  if (server.client())
    server.send(200, "text/plain", "Balança zerada com sucesso.");
}

// ─────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────
void setup()
{
  Serial.begin(115200);

  NEXTION_SERIAL.begin(NEXTION_BAUD, SERIAL_8N1, 16, 17);
  delay(500);
  nextionCmd("page 0");
  nextionCmd("tPeso.txt=\"---\"");

  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
  Serial.println("IP: " + WiFi.localIP().toString());

  if (MDNS.begin("balanca"))
    MDNS.addService("http", "tcp", 80);

  server.on("/",         handleRoot);
  server.on("/dados",    handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/zero",     handleZero);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Não encontrado.");
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

// ─────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────
void loop()
{
  server.handleClient();
  lerNextion();

  if (scale.is_ready())
    pesoAtual = scale.get_units(5);

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
    Serial.printf("Peso: %.2f g | Fator: %.4f\n",
                  pesoAtual, calibration_factor);
  }
}