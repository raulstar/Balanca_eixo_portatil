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
            align-items: flex-start;
            padding: 18px 24px 10px 24px;
            background: #0a0e27;
        }

        .logo {
            font-size: 28px;
            font-weight: bold;
            color: #ff3333;
            line-height: 1;
        }

        .logo-sub {
            font-size: 10px;
            color: #888;
            letter-spacing: 2px;
            margin-top: 3px;
            text-transform: uppercase;
        }

        .header-right {
            display: flex;
            flex-direction: column;
            align-items: flex-end;
            gap: 2px;
        }

        .header-info-row {
            display: flex;
            gap: 24px;
            align-items: flex-start;
        }

        .header-info-block {
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .header-info-title {
            font-size: 9px;
            color: #555;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 2px;
        }

        .main-content {
            flex: 1;
            display: flex;
            flex-direction: column;
            justify-content: center;
            padding: 10px 16px;
            gap: 10px;
        }

        .panel {
            background: #131729;
            border-radius: 10px;
            padding: 18px 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border: 1px solid rgba(255,255,255,0.05);
        }

        .panel-left {
            display: flex;
            flex-direction: column;
            gap: 4px;
            text-align: right;
            align-items: flex-end;
        }

        .panel-label {
            font-size: 10px;
            color: #555;
            text-transform: uppercase;
            letter-spacing: 1.5px;
        }

        .panel-value {
            font-size: 42px;
            font-weight: bold;
            color: #4488ff;
            line-height: 1;
            letter-spacing: 1px;
        }

        .panel-value.green {
            color: #00e676;
        }

        .panel-unit {
            font-size: 16px;
            font-weight: normal;
            color: #555;
            margin-left: 6px;
            vertical-align: bottom;
            line-height: 1.8;
        }

        .btn-zero {
            background: #1e2340;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 18px 32px;
            font-size: 15px;
            font-weight: bold;
            letter-spacing: 1.5px;
            cursor: pointer;
            text-transform: uppercase;
            transition: opacity 0.2s;
            min-width: 120px;
        }

        .btn-zero:hover { opacity: 0.8; }

        .btn-somar {
            background: #ff3333;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 18px 32px;
            font-size: 15px;
            font-weight: bold;
            letter-spacing: 1.5px;
            cursor: pointer;
            text-transform: uppercase;
            transition: opacity 0.2s;
            min-width: 120px;
        }

        .btn-somar:hover { opacity: 0.8; }

        .bottom-row {
            display: flex;
            gap: 10px;
            padding: 6px 16px 20px 16px;
        }

        .btn-bottom {
            flex: 1;
            background: #131729;
            color: #aaa;
            border: 1px solid rgba(255,255,255,0.07);
            border-radius: 8px;
            padding: 15px 10px;
            font-size: 12px;
            font-weight: bold;
            letter-spacing: 1.5px;
            cursor: pointer;
            text-transform: uppercase;
            transition: opacity 0.2s, background 0.2s;
        }

        .btn-bottom:hover { background: #1e2340; opacity: 1; }

        #status {
            text-align: center;
            font-size: 12px;
            color: #555;
            font-style: italic;
            padding: 0 16px 8px 16px;
            min-height: 18px;
        }
    </style>
</head>
<body>

    <header class="header">
        <div>
            <div class="logo">REVLO</div>
            <div class="logo-sub">Sistema de Pesagem</div>
        </div>
        <div class="header-right">
            <div class="header-info-row">
                <div class="header-info-block">
                    <span class="header-info-title">ID Dispositivo</span>
                </div>
                <div class="header-info-block">
                    <span class="header-info-title">Bateria</span>
                </div>
            </div>
        </div>
    </header>

    <main class="main-content">

        <div class="panel">
            <button class="btn-zero" onclick="zerar()">ZERO</button>
            <div class="panel-left">
                <span class="panel-label">Peso Atual</span>
                <span>
                    <span class="panel-value" id="peso">--</span>
                    <span class="panel-unit">KG</span>
                </span>
            </div>
        </div>

        <div class="panel">
            <button class="btn-somar">SOMAR</button>
            <div class="panel-left">
                <span class="panel-label">Peso Total Acumulado</span>
                <span>
                    <span class="panel-value green" id="pesoAcumulado">0.000</span>
                    <span class="panel-unit">KG</span>
                </span>
            </div>
        </div>

    </main>

    <div id="status"></div>

    <div class="bottom-row">
        <button class="btn-bottom" onclick="calibrar()">Configurações</button>
        <button class="btn-bottom">Ajuda</button>
        <button class="btn-bottom">Histórico</button>
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
                    document.getElementById('peso').innerText = peso;
                })
                .catch(() => setStatus('Erro na conexão com a balança...'));
        }

        function calibrar() {
    // 1. Pergunta o peso ao usuário
    const peso = window.prompt("Insira o peso (em gramas) que está sobre a balança:", "100");
    
    // 2. Valida se o usuário não cancelou ou digitou algo inválido
    if (peso === null || peso === "" || isNaN(peso)) {
        setStatus("Calibração cancelada ou valor inválido.");
        return;
    }

    const pesoFloat = parseFloat(peso);
    setStatus("Enviando comando de calibração (" + pesoFloat + "g)...");

    // 3. Faz a requisição para o ESP32
    fetch('/calibrar?peso=' + pesoFloat)
        .then(response => {
            if (response.ok) return response.text();
            throw new Error('Falha no servidor');
        })
        .then(msg => {
            setStatus(msg); // Exibe "Calibrando... Verifique o display"
        })
        .catch(err => {
            setStatus("Erro ao iniciar calibração: " + err.message);
        });
}
            setStatus('Calibrando... aguarde ~10s.');
            fetch('/calibrar?peso=' + parseFloat(peso))
                .then(r => r.text())
                .then(msg => setStatus(msg))
                .catch(() => setStatus('Erro ao enviar calibração.'));
        }

        function zerar() {
            setStatus('Zerando balança...');
            
            fetch('/zero')
                .then(r => {
                    if (r.ok) return r.text();
                    throw new Error('Erro de servidor');
                })
                .then(msg => {
                    setStatus(msg);
                    setTimeout(() => setStatus(''), 3000); // apaga o status em 3s
                })
                .catch(() => setStatus('Erro ao executar tara.'));
        }

        setInterval(buscarDados, 500);
    </script>
</body>
</html>
)rawliteral";

#endif