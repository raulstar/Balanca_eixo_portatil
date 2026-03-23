#include <Arduino.h>
#include "HX711.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "PaginaHTML.h"

/////////////////////////////////////////////////////////////////////////////
//  PINOS
#define DOUT_PIN 4
#define SCK_PIN  5

#define NEXTION_SERIAL Serial2
#define NEXTION_BAUD   9600

/////////////////////////////////////////////////////////////////////////////
//  OBJETOS E VARIÁVEIS GLOBAIS
HX711     scale;
WebServer server(80);

const char *ssid     = "Revlo_Claro";
const char *password = "Revlo@2025";

float calibration_factor = 88.0706;
float pesoAtual          = 0.0;
bool  zero               = false;
const int   N          = 10;
unsigned long ultimaAtualizacaoTela = 0;
const unsigned long INTERVALO_TELA  = 200;

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

/////////////////////////////////////////////////////////////////////////////
//  LOOP
void loop()
{
  server.handleClient();
  lerNextion();

  if (scale.is_ready())
  {
    float medida = scale.get_units(1);  // leitura bruta do HX711
    zero         = calculoDeZero(medida); // avalia a leitura ATUAL, não a anterior
    pesoAtual    = filtro(medida);        // filtra com zero já atualizado
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
    Serial.printf("Peso: %.2f g \n", pesoAtual);
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
  snprintf(buffer, sizeof(buffer), "%.2f g", pesoAtual);
  nextionCmd(String("tPeso.txt=\"") + buffer + "\"");
}

/////////////////////////////////////////////////////////////////////////////
//  CALIBRAÇÃO
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

/////////////////////////////////////////////////////////////////////////////
//  NEXTION — LÊ EVENTOS DE TOQUE
void lerNextion()
{
  if (!NEXTION_SERIAL.available()) return;

  String entrada = NEXTION_SERIAL.readStringUntil('\xFF');
  entrada.trim();

  if (entrada.length() == 0) return;

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

/////////////////////////////////////////////////////////////////////////////
//  HANDLERS DO SERVIDOR WEB
void handleRoot()
{
  server.send_P(200, "text/html", pagina_html);
}
/////////////////////////////////////////////////////////////////////////////

void handleDados()
{
  String json = "{";
  json += "\"pesoAtual\":"          + String(max(0.0f, pesoAtual), 4) + ",";
  json += "\"calibration_factor\":" + String(calibration_factor, 4);
  json += "}";
  server.send(200, "application/json", json);
}
/////////////////////////////////////////////////////////////////////////////

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
/////////////////////////////////////////////////////////////////////////////

void handleZero()
{
  scale.tare();
  pesoAtual = 0.0;
  Serial.println("Tara realizada.");

  // Responde imediatamente ao navegador
  if (server.client()) {
    server.send(200, "text/plain", "Balança zerada com sucesso!");
  }

  // Comandos da tela física
  nextionCmd("tPeso.txt=\"Zerado!\"");
  delay(800);
}
/////////////////////////////////////////////////////////////////////////////
bool calculoDeZero(float peso)
{
  const float PESO_MINIMO = 1.0; // gramas
  return (peso >= PESO_MINIMO);
}
/////////////////////////////////////////////////////////////////////////////

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

  //Serial.printf("[filtro] Media: %.4f | Desvio Padrao: %.4f\n", media, desvioPadrao);

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

  //Serial.printf("[filtro] Moda: %.4f | Grupo: %d amostras\n", melhorCentro, melhorContagem);

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

  //Serial.printf("[filtro] Resultado: %.4f (usou %d/%d amostras)\n",
  //              ultimoValido, countFiltrado, N);

  count = 0; // reseta buffer
  return ultimoValido;
}

