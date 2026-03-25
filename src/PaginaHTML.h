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
            font-family: sans-serif;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }

        .header {
            display: flex;
            justify-content: space-between;
            padding: 18px 24px;
        }

        .logo {
            font-size: 28px;
            font-weight: bold;
            color: #ff3333;
        }

        .logo-sub {
            font-size: 10px;
            color: #888;
        }

        .main-content {
            flex: 1;
            display: flex;
            flex-direction: column;
            justify-content: center;
            padding: 10px;
            gap: 10px;
        }

        .panel {
            background: #131729;
            border-radius: 10px;
            padding: 18px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .panel-value {
            font-size: 42px;
            font-weight: bold;
            color: #4488ff;
        }

        .panel-value.green {
            color: #00e676;
        }

        .panel-unit {
            font-size: 16px;
            color: #555;
        }

        button {
            border: none;
            border-radius: 8px;
            padding: 15px;
            font-weight: bold;
            cursor: pointer;
        }

        .btn-zero,
.btn-somar {
    flex: 1;
    min-width: 120px;
}

        .btn-somar {
            background: #ff3333;
            color: white;
        }

        .bottom-row {
            display: flex;
            gap: 10px;
            padding: 10px;
        }

        .btn-bottom {
            flex: 1;
            background: #131729;
            color: #aaa;
        }

        #status {
            text-align: center;
            font-size: 12px;
            color: #555;
            padding: 5px;
        }
    </style>
</head>

<body>

<header class="header">
    <div>
        <div class="logo">REVLO</div>
        <div class="logo-sub">Sistema de Pesagem</div>
    </div>
</header>

<main class="main-content">

    <div class="panel">
        <button class="btn-zero" onclick="zerar()">ZERO</button>
        <div>
            <div>Peso Atual</div>
            <span class="panel-value" id="peso">--</span>
            <span class="panel-unit">KG</span>
        </div>
    </div>

    <div class="panel">
        <button class="btn-somar">SOMAR</button>
        <div>
            <div>Total</div>
            <span class="panel-value green" id="pesoAcumulado">0.000</span>
            <span class="panel-unit">KG</span>
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
            .catch(() => setStatus('Erro na conexão...'));
    }

    // 🔥 NOVA FUNÇÃO SIMPLES (sem prompt)
    function calibrar() {
        setStatus("Iniciando calibração...");

        fetch('/calibrar')
            .then(r => r.text())
            .then(msg => setStatus(msg))
            .catch(() => setStatus("Erro ao calibrar"));
    }

    function zerar() {
        setStatus('Zerando...');

        fetch('/zero')
            .then(r => r.text())
            .then(msg => {
                setStatus(msg);
                setTimeout(() => setStatus(''), 2000);
            })
            .catch(() => setStatus('Erro ao zerar'));
    }

    setInterval(buscarDados, 200);
</script>

</body>
</html>
)rawliteral";

#endif