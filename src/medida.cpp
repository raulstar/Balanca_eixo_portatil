#include "medida.h"

// ─── Construtor ───────────────────────────────────────────────────
Medida::Medida(uint8_t dout, uint8_t sck)
  : _dout(dout), _sck(sck),
    _bufferIndex(0), _bufferFilled(false),
    _emaValue(0.0f), _emaInitialized(false)
{
  memset(_buffer, 0, sizeof(_buffer));
}

// ─── Inicialização ────────────────────────────────────────────────
void Medida::begin(float scaleFactor) {
  _scale.begin(_dout, _sck);

  // Aguarda o HX711 ficar pronto
  while (!_scale.is_ready()) {
    Serial.println("[Medida] Aguardando HX711...");
    delay(200);
  }

  _scale.set_scale(scaleFactor);
  _scale.tare();

  Serial.println("[Medida] HX711 inicializado e zerado.");
}

// ─── isReady ─────────────────────────────────────────────────────
bool Medida::isReady() {
  return _scale.is_ready();
}

// ─── Leitura digital bruta do ADC ────────────────────────────────
long Medida::readDigitalRaw() {
  return _scale.read();
}

// ─── Leitura em gramas sem filtro ────────────────────────────────
float Medida::readRawGrams() {
  return _scale.get_units(1);
}

// ─── Tare manual ─────────────────────────────────────────────────
void Medida::tare() {
  _scale.tare();
  resetFilter(); // Reseta o filtro após o zero
  Serial.println("[Medida] Tare aplicado e filtro resetado.");
}

// ─── Último valor EMA sem nova leitura ───────────────────────────
float Medida::getLastFiltered() {
  return _emaInitialized ? _emaValue : 0.0f;
}

// ─── Média do buffer ─────────────────────────────────────────────
float Medida::getBufferMean() {
  int size = _bufferFilled ? BUFFER_SIZE : _bufferIndex;
  if (size == 0) return 0.0f;
  return _calcMean(_buffer, size);
}

// ─── Desvio padrão do buffer ─────────────────────────────────────
float Medida::getBufferStdDev() {
  int size = _bufferFilled ? BUFFER_SIZE : _bufferIndex;
  if (size < 2) return 0.0f;
  return _calcStdDev(_buffer, size, _calcMean(_buffer, size));
}

// ─── Reset do filtro ─────────────────────────────────────────────
void Medida::resetFilter() {
  memset(_buffer, 0, sizeof(_buffer));
  _bufferIndex    = 0;
  _bufferFilled   = false;
  _emaValue       = 0.0f;
  _emaInitialized = false;
}

// ─── Filtro principal: Outlier Rejection + EMA ───────────────────
float Medida::readFiltered() {
  float newReading = _scale.get_units(1);
  int   size       = _bufferFilled ? BUFFER_SIZE : _bufferIndex;

  // Com pelo menos 2 amostras no buffer, verifica outlier
  if (size >= 2 && _isOutlier(newReading)) {
    float mean   = getBufferMean();
    float stdDev = getBufferStdDev();
    Serial.printf("[OUTLIER] %.2f g descartado (média=%.2f g, σ=%.2f)\n",
                  newReading, mean, stdDev);
    // Retorna último EMA sem atualizar o buffer
    return _emaInitialized ? _emaValue : mean;
  }

  // Adiciona no buffer circular
  _addToBuffer(newReading);

  // Aplica EMA e retorna
  return _applyEMA(newReading);
}

// ═══════════════════════════════════════════════════════════════════
// MÉTODOS PRIVADOS
// ═══════════════════════════════════════════════════════════════════

float Medida::_calcMean(float* buf, int size) {
  float sum = 0.0f;
  for (int i = 0; i < size; i++) sum += buf[i];
  return sum / size;
}

float Medida::_calcStdDev(float* buf, int size, float mean) {
  float variance = 0.0f;
  for (int i = 0; i < size; i++) {
    float diff = buf[i] - mean;
    variance  += diff * diff;
  }
  return sqrt(variance / size);
}

bool Medida::_isOutlier(float reading) {
  int   size   = _bufferFilled ? BUFFER_SIZE : _bufferIndex;
  float mean   = _calcMean(_buffer, size);
  float stdDev = _calcStdDev(_buffer, size, mean);
  return fabs(reading - mean) > (OUTLIER_THRESHOLD * stdDev);
}

void Medida::_addToBuffer(float value) {
  _buffer[_bufferIndex] = value;
  _bufferIndex          = (_bufferIndex + 1) % BUFFER_SIZE;
  if (_bufferIndex == 0) _bufferFilled = true;
}

float Medida::_applyEMA(float value) {
  if (!_emaInitialized) {
    _emaValue       = value;
    _emaInitialized = true;
  } else {
    _emaValue = EMA_ALPHA * value + (1.0f - EMA_ALPHA) * _emaValue;
  }
  return _emaValue;
}