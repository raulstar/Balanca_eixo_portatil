#ifndef PAGINA_HTML_H
#define PAGINA_HTML_H

const char pagina_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>ESP32</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f4f4f4;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
    }
    .card {
      background: #fff;
      padding: 30px 40px;
      border-radius: 10px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.2);
      text-align: center;
      min-width: 320px;
    }
    h1 { color: #333; margin-bottom: 20px; }
    .campo {
      background: #f0f0f0;
      border-radius: 8px;
      padding: 12px 20px;
      margin: 10px 0;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .label   { color: #666; font-size: 14px; }
    .valor   { color: #007bff; font-size: 20px; font-weight: bold; }
    .unidade { color: #999; font-size: 13px; margin-left: 4px; }

    .input-group {
      display: flex;
      gap: 8px;
      margin: 10px 0;
    }
    .input-group input {
      flex: 1;
      padding: 10px;
      border: 1px solid #ccc;
      border-radius: 5px;
      font-size: 15px;
      text-align: center;
    }

    .btn-row {
      display: flex;
      gap: 10px;
      margin-top: 8px;
    }
    button {
      padding: 10px;
      border: none;
      border-radius: 5px;
      font-size: 15px;
      cursor: pointer;
      flex: 1;
      color: white;
    }
    .btn-calibrar       { background: #28a745; }
    .btn-calibrar:hover { background: #1e7e34; }
    .btn-zero           { background: #dc3545; }
    .btn-zero:hover     { background: #a71d2a; }

    #status {
      margin-top: 14px;
      font-size: 13px;
      color: #555;
      min-height: 18px;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>Balança de eixo Revlo</h1>

    <div class="campo">
      <span class="label">Peso Atual</span>
      <span>
        <span class="valor" id="peso">--</span>
        <span class="unidade">g</span>
      </span>
    </div>

    <div class="campo">
      <span class="label">Fator de Calibração</span>
      <span>
        <span class="valor" id="calibracao">--</span>
      </span>
    </div>

    <div class="input-group">
      <input type="number" id="pesoConhecido" placeholder="Peso conhecido (g)" step="0.01" min="0">
    </div>

    <div class="btn-row">
      <button class="btn-calibrar" onclick="calibrar()">Calibrar</button>
      <button class="btn-zero"     onclick="zerar()">Zero (Tara)</button>
    </div>

    <div id="status"></div>
  </div>

  <script>
    function setStatus(msg) {
      document.getElementById('status').innerText = msg;
    }

    function buscarDados() {
      fetch('/dados')
        .then(r => r.json())
        .then(d => {
          const peso = d.pesoAtual !== null
            ? Math.max(0, d.pesoAtual).toFixed(2)
            : '--';
          document.getElementById('peso').innerText      = peso;
          document.getElementById('calibracao').innerText = d.calibration_factor.toFixed(4);
        })
        .catch(() => setStatus('Erro ao buscar dados.'));
    }

    function calibrar() {
      const peso = document.getElementById('pesoConhecido').value;
      if (!peso || parseFloat(peso) <= 0) {
        setStatus('Informe um peso conhecido valido.');
        return;
      }
      setStatus('Calibrando... aguarde ~10s.');
      fetch('/calibrar?peso=' + parseFloat(peso))
        .then(r => r.text())
        .then(msg => setStatus(msg))
        .catch(() => setStatus('Erro ao calibrar.'));
    }

    function zerar() {
      setStatus('Zerando...');
      fetch('/zero')
        .then(r => r.text())
        .then(msg => setStatus(msg))
        .catch(() => setStatus('Erro ao zerar.'));
    }

    setInterval(buscarDados, 500);
  </script>
</body>
</html>
)rawliteral";

#endif