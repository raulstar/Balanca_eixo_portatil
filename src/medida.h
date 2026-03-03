#ifndef MEDIDA_H
#define MEDIDA_H

#include <Arduino.h>
#include <HX711.h>

// ─── Parâmetros do filtro ─────────────────────────────────────────
#define BUFFER_SIZE        10
#define OUTLIER_THRESHOLD  2.0f
#define EMA_ALPHA          0.1f

class Medida {
public:
  // Construtor: recebe os pinos DOUT e SCK do HX711
  Medida(uint8_t dout, uint8_t sck);

  // Inicializa o HX711, aplica escala e tare
  void  begin(float scaleFactor);

  // Verifica se o HX711 está pronto para leitura
  bool  isReady();

  // Leitura digital bruta do ADC (sem escala, sem tare)
  long  readDigitalRaw();

  // Leitura em gramas sem filtro (com escala e tare aplicados)
  float readRawGrams();

  // Outlier rejection + EMA — valor final filtrado
  float readFiltered();

  // Zera a balança (aplica tare manualmente)
  void  tare();

  // Retorna o último valor EMA calculado (sem nova leitura)
  float getLastFiltered();

  // Retorna a média atual do buffer
  float getBufferMean();

  // Retorna o desvio padrão atual do buffer
  float getBufferStdDev();

  // Reseta o buffer e o EMA (útil após tare manual)
  void  resetFilter();

private:
  HX711   _scale;
  uint8_t _dout;
  uint8_t _sck;

  // Buffer circular para outlier rejection
  float _buffer[BUFFER_SIZE];
  int   _bufferIndex;
  bool  _bufferFilled;

  // Estado do EMA
  float _emaValue;
  bool  _emaInitialized;

  // Helpers internos
  float _calcMean(float* buf, int size);
  float _calcStdDev(float* buf, int size, float mean);
  bool  _isOutlier(float reading);
  void  _addToBuffer(float value);
  float _applyEMA(float value);
};

#endif