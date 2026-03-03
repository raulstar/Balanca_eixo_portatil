#ifndef MEDIDA_H
#define MEDIDA_H

#include <Arduino.h>
#include <HX711.h>

// ─── Parâmetros do filtro ─────────────────────────────────────────
#define BUFFER_SIZE        10
#define OUTLIER_THRESHOLD  2.0f
#define EMA_ALPHA          0.1f
#define CALIBRATION_SAMPLES 20   // Amostras usadas durante a calibração

class Medida {
public:
  Medida(uint8_t dout, uint8_t sck);

  void  begin(float scaleFactor);
  bool  isReady();

  long  readDigitalRaw();
  float readRawGrams();
  float readFiltered();

  void  tare();
  float getLastFiltered();
  float getBufferMean();
  float getBufferStdDev();
  void  resetFilter();

  // Aplica um novo fator de escala vindo de fora (ex: EEPROM ou calibração)
  void  setScaleFactor(float fator);

  // Retorna leitura bruta do ADC sem escala — usada no cálculo do fator
  long  getRawAdcValue(int amostras = CALIBRATION_SAMPLES);

private:
  HX711   _scale;
  uint8_t _dout;
  uint8_t _sck;

  float _buffer[BUFFER_SIZE];
  int   _bufferIndex;
  bool  _bufferFilled;
  float _emaValue;
  bool  _emaInitialized;

  float _calcMean(float* buf, int size);
  float _calcStdDev(float* buf, int size, float mean);
  bool  _isOutlier(float reading);
  void  _addToBuffer(float value);
  float _applyEMA(float value);
};

#endif