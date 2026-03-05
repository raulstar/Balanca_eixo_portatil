#include <Arduino.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

#define DOUT_PIN 4
#define SCK_PIN 5

// Objetos e Variáveis Globais
HX711 scale;
WebServer server(80);

const char *ssid = "REVLO";
const char *password = "Revlo!2024";

float calibration_factor = 420.0;
float pesoAtual = 0;

// --- FUNÇÕES DE CONTROLE ---

void realizarCalibracao(float pesoConhecido) {
  Serial.println("\n--- INICIANDO CALIBRAÇÃO ---");
  
  scale.set_scale(); // Reseta a escala para 1.0 para medir o valor bruto
  scale.tare();      // Zera a balança sem peso
  
  Serial.print("Coloque o peso de ");
  Serial.print(pesoConhecido);
  Serial.println("g. Aguardando 5s para estabilizar...");
  delay(5000);

  // Pega a média de 20 leituras para uma calibração precisa
  float leituraBruta = scale.get_units(20);
  calibration_factor = leituraBruta / pesoConhecido;

  scale.set_scale(calibration_factor);

  Serial.println("--- CALIBRAÇÃO CONCLUÍDA ---");
  Serial.print("Novo Fator: ");
  Serial.println(calibration_factor);
}

// --- HANDLERS DO SERVIDOR WEB ---

void handleRoot() {
  server.send_P(200, "text/html", pagina_html);
}

void handleDados() {
  // Criando o JSON com o peso atualizado no momento da requisição
  String json = "{";
  json += "\"pesoAtual\":" + String(pesoAtual, 4) + ",";
  json += "\"calibration_factor\":" + String(calibration_factor, 4);
  json += "}";
  server.send(200, "application/json", json);
}

void handleCalibrar() {
  if (server.hasArg("peso")) {
    float pesoConhecido = server.arg("peso").toFloat();
    if (pesoConhecido > 0) {
      realizarCalibracao(pesoConhecido);
      server.send(200, "text/plain", "Calibrado com " + String(pesoConhecido) + "g");
      return;
    }
  }
  server.send(400, "text/plain", "Peso invalido");
}

void handleZero() {
  scale.tare(); // A biblioteca limpa o offset interno
  pesoAtual = 0; // Zera a variável imediatamente para o próximo ciclo
  Serial.println("Tara realizada via Web.");
  server.send(200, "text/plain", "Balança zerada com sucesso.");
}

void setup() {
  Serial.begin(115200);
  
  // Inicializa o HX711
  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  // Conexão WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // Configuração mDNS (acesso via http://balanca.local)
  if (MDNS.begin("balanca")) {
    MDNS.addService("http", "tcp", 80);
  }

  // Rotas do Servidor
  server.on("/", handleRoot);
  server.on("/dados", handleDados);
  server.on("/calibrar", handleCalibrar);
  server.on("/zero", handleZero);
  server.onNotFound([]() { server.send(404, "text/plain", "Nao encontrado"); });
  
  server.begin();
  Serial.println("Servidor pronto em: " + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
  
  // Otimização: Apenas 10 amostras para manter a fluidez do site
  // Se scale.is_ready() for falso, get_units pula a leitura para não travar o loop
  if (scale.is_ready()) {
    pesoAtual = scale.get_units(10); 
  }

  // Monitoramento Serial (opcional)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    Serial.printf("Peso: %.2f g\n", pesoAtual);
    lastPrint = millis();
  }
}