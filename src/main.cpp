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

// Nextion via UART2: RX=16, TX=17
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
float pesoAtual          = 0;
bool  zero               = 0;
bool  calibrando         = 0;

// Controle de intervalo de atualização do display
unsigned long ultimaAtualizacaoTela = 0;
const unsigned long INTERVALO_TELA  = 200; // ms

// ─────────────────────────────────────────
//  NEXTION — FUNÇÕES AUXILIARES
// ─────────────────────────────────────────

// Envia qualquer comando ao Nextion (sempre termina com 3x 0xFF)
void nextionCmd(const String &cmd)
{
  NEXTION_SERIAL.print(cmd);
  NEXTION_SERIAL.write(0xFF);
  NEXTION_SERIAL.write(0xFF);
  NEXTION_SERIAL.write(0xFF);
}

// ─────────────────────────────────────────
//  NEXTION — lê eventos de toque
//
//  No Nextion Editor, configure os botões assim:
//    bCalib  → Touch Release Event → printh 43 41 4C 49 42 FF FF FF
//    bZero   → Touch Release Event → printh 5A 45 52 4F FF FF FF
//  (envia as strings "CALIB" e "ZERO" pela serial)
// ─────────────────────────────────────────
void lerNextion()
{
  if (!NEXTION_SERIAL.available()) return;

  // Lê até o primeiro 0xFF (terminador Nextion)
  String entrada = NEXTION_SERIAL.readStringUntil('\xFF');
  entrada.trim();

  if (entrada.length() == 0) return;

  if (entrada == "CALIB")
  {
    calibrando = !calibrando;          // alterna estado
    Serial.printf("[Nextion] Calibrando: %s\n", calibrando ? "ATIVO" : "INATIVO");

    // Feedback visual: muda o texto do botão na tela
    nextionCmd(calibrando
               ? "bCalib.txt=\"Calibr. ON\""
               : "bCalib.txt=\"Calibracao\"");
  }
  else if (entrada == "ZERO")
  {
    zero = !zero;                      // alterna estado
    Serial.printf("[Nextion] Zero: %s\n", zero ? "ATIVO" : "INATIVO");

    // Feedback visual: muda o texto do botão na tela
    nextionCmd(zero
               ? "bZero.txt=\"Zero ON\""
               : "bZero.txt=\"Zero\"");
  }
}

// ─────────────────────────────────────────
//  NEXTION — atualiza o peso exibido
//
//  Componente na tela: Text  nome=tPeso  id=1
//  Exibe pesoAtual com 2 casas decimais + "g"
// ─────────────────────────────────────────
void atualizarPesoNaTela()
{
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.2f g", pesoAtual);

  String cmd = String("tPeso.txt=\"") + buffer + "\"";
  nextionCmd(cmd);
}

// ─────────────────────────────────────────
//  FUNÇÕES DE CONTROLE (original)
// ─────────────────────────────────────────

void realizarCalibracao(float pesoConhecido)
{
  Serial.println("\n--- INICIANDO CALIBRAÇÃO ---");

  scale.set_scale();
  scale.tare();

  Serial.print("Coloque o peso de ");
  Serial.print(pesoConhecido);
  Serial.println("g. Aguardando 5s para estabilizar...");

  // Informa na tela que está aguardando
  nextionCmd("tPeso.txt=\"Aguarde...\"");
  delay(5000);

  float leituraBruta    = scale.get_units(20);
  calibration_factor    = leituraBruta / pesoConhecido;

  scale.set_scale(calibration_factor);

  Serial.println("--- CALIBRAÇÃO CONCLUÍDA ---");
  Serial.print("Novo Fator: ");
  Serial.println(calibration_factor);

  // Sinaliza conclusão na tela por 1s, depois retorna ao peso
  nextionCmd("tPeso.txt=\"Calibrado!\"");
  delay(1000);
}

// ─────────────────────────────────────────
//  HANDLERS DO SERVIDOR WEB (original)
// ─────────────────────────────────────────

void handleRoot()
{
  server.send_P(200, "text/html", pagina_html);
}

void handleDados()
{
  String pesoStr = (zero && pesoAtual > 0) ? String(pesoAtual, 4) : "null";
  String json    = "{";
  json += "\"pesoAtual\":"          + pesoStr                        + ",";
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
                  "Calibrado com " + String(pesoConhecido) + "g");
      return;
    }
  }
  server.send(400, "text/plain", "Peso invalido");
}

void handleZero()
{
  scale.tare();
  pesoAtual = 0;
  Serial.println("Tara realizada via Web.");
  server.send(200, "text/plain", "Balança zerada com sucesso.");
}

// ─────────────────────────────────────────
//  LÓGICA DE ZERO (original)
// ─────────────────────────────────────────

bool calculoDeZero(float pesoAtual)
{
  const float PESO_MINIMO = 1.0;
  return (pesoAtual >= PESO_MINIMO);
}

// ─────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────

void setup()
{
  Serial.begin(115200);

  // Nextion
  NEXTION_SERIAL.begin(NEXTION_BAUD, SERIAL_8N1, 16, 17); // RX=16, TX=17
  delay(500);                    // aguarda Nextion inicializar
  nextionCmd("page 0");          // garante que está na page0
  nextionCmd("tPeso.txt=\"---\"");

  // HX711
  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // mDNS
  if (MDNS.begin("balanca"))
    MDNS.addService("http", "tcp", 80);

  // Rotas Web
  server.on("/",         handleRoot);
  server.on("/dados",    handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/zero",     handleZero);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Nao encontrado");
  });

  server.begin();
  Serial.println("Servidor pronto em: " + WiFi.localIP().toString());
}

// ─────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────

void loop()
{
  // Web
  server.handleClient();

  // Nextion — lê eventos de toque
  lerNextion();

  // Lógica de zero
  zero = calculoDeZero(pesoAtual);

  // HX711 — leitura da célula de carga
  if (scale.is_ready())
    pesoAtual = scale.get_units(5);

  // Nextion — atualiza peso na tela a cada INTERVALO_TELA ms
  unsigned long agora = millis();
  if (agora - ultimaAtualizacaoTela >= INTERVALO_TELA)
  {
    ultimaAtualizacaoTela = agora;
    atualizarPesoNaTela();
  }

  // Serial monitor
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500)
  {
    Serial.printf("Peso: %.2f g | zero: %d | calibrando: %d\n",
                  pesoAtual, zero, calibrando);
    lastPrint = millis();
  }
}