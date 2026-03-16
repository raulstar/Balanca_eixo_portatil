#ifndef PAGINA_HTML_H
#define PAGINA_HTML_H

const char pagina_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 - Balança Revlo</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            background-color: #0a0e27;
            color: #ffffff;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 20px 40px;
            background: rgba(255, 255, 255, 0.05);
        }

        .logo {
            font-size: 24px;
            font-weight: bold;
            color: #ff4444;
        }

        .header-right {
            display: flex;
            gap: 20px;
            font-size: 12px;
            color: #888;
        }

        .main-content {
            flex: 1;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }

        .card {
            background: #1a1f3d;
            padding: 40px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            text-align: center;
            width: 100%;
            max-width: 450px;
            border: 1px solid rgba(255,255,255,0.1);
        }

        h1 {
            color: #ffffff;
            margin-bottom: 30px;
            font-size: 22px;
            letter-spacing: 1px;
        }

        .campo {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 12px;
            padding: 18px 25px;
            margin: 15px 0;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border: 1px solid rgba(255,255,255,0.05);
        }

        .label { color: #aaa; font-size: 14px; text-transform: uppercase; letter-spacing: 1px; }
        .valor { color: #4488ff; font-size: 26px; font-weight: bold; }
        .unidade { color: #666; font-size: 14px; margin-left: 5px; }

        .input-group {
            margin: 25px 0 15px 0;
        }

        .input-group input {
            width: 100%;
            padding: 15px;
            background: #0a0e27;
            border: 1px solid #333;
            border-radius: 8px;
            font-size: 16px;
            text-align: center;
            color: white;
            outline: none;
        }

        .input-group input:focus {
            border-color: #4488ff;
        }

        .btn-row {
            display: flex;
            gap: 15px;
            margin-top: 10px;
        }

        button {
            padding: 15px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
            cursor: pointer;
            flex: 1;
            color: white;
            text-transform: uppercase;
            transition: opacity 0.2s;
        }

        .btn-calibrar { background: #28a745; }
        .btn-zero     { background: #dc3545; }
        button:hover  { opacity: 0.8; }

        #status {
            margin-top: 20px;
            font-size: 13px;
            color: #888;
            min-height: 20px;
            font-style: italic;
        }

        .footer {
            padding: 20px;
            text-align: center;
            font-size: 11px;
            color: #444;
            background: rgba(0,0,0,0.2);
        }
    </style>
</head>
<body>

    <header class="header">
        <div class="logo">REVLO</div>
        <div class="header-right">
            <span>SISTEMA DE PESAGEM v2.0</span>
            <span>ESP32 CONNECTED</span>
        </div>
    </header>

    <main class="main-content">
        <div class="card">
            <h1>Balança de Eixo</h1>

            <div class="campo">
                <span class="label">Peso Atual</span>
                <span>
                    <span class="valor" id="peso">--</span>
                    <span class="unidade">g</span>
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
    </main>

    <footer class="footer">
        &copy; 2024 REVLO TECNOLOGIA - TODOS OS DIREITOS RESERVADOS
    </footer>

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
                    document.getElementById('peso').innerText = peso;
                })
                .catch(() => setStatus('Erro na conexão com a balança...'));
        }

        function calibrar() {
            const peso = document.getElementById('pesoConhecido').value;
            if (!peso || parseFloat(peso) <= 0) {
                setStatus('Informe um peso conhecido válido.');
                return;
            }
            setStatus('Calibrando... aguarde ~10s.');
            fetch('/calibrar?peso=' + parseFloat(peso))
                .then(r => r.text())
                .then(msg => setStatus(msg))
                .catch(() => setStatus('Erro ao enviar calibração.'));
        }

        function zerar() {
            setStatus('Zerando escala...');
            fetch('/zero')
                .then(r => r.text())
                .then(msg => setStatus(msg))
                .catch(() => setStatus('Erro ao executar tara.'));
        }

        setInterval(buscarDados, 500);
    </script>
</body>
</html>
)rawliteral";

#endif