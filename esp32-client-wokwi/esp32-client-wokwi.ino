#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

WebServer server(80);

void sendHtml() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Pet Dashboard</title>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;500;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-color: #0f172a;
      --card-bg: rgba(255, 255, 255, 0.05);
      --card-border: rgba(255, 255, 255, 0.1);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --accent: #3b82f6;
      --accent-hover: #2563eb;
      --danger: #ef4444;
      --success: #10b981;
    }

    * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Outfit', sans-serif; }

    body {
      background: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 2rem;
    }

    header {
      text-align: center;
      margin-bottom: 2rem;
    }

    h1 {
      font-size: 2.5rem;
      font-weight: 700;
      background: -webkit-linear-gradient(45deg, #3b82f6, #8b5cf6);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      margin-bottom: 0.5rem;
    }

    .setup-container {
      background: var(--card-bg);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid var(--card-border);
      border-radius: 16px;
      padding: 1.5rem;
      margin-bottom: 2rem;
      display: flex;
      gap: 1rem;
      align-items: center;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }

    input {
      background: rgba(0, 0, 0, 0.2);
      border: 1px solid var(--card-border);
      color: white;
      padding: 0.8rem 1rem;
      border-radius: 8px;
      font-size: 1rem;
      outline: none;
      transition: border-color 0.3s;
    }

    input:focus { border-color: var(--accent); }

    button {
      background: var(--accent);
      color: white;
      border: none;
      padding: 0.8rem 1.5rem;
      border-radius: 8px;
      font-size: 1rem;
      font-weight: 500;
      cursor: pointer;
      transition: background 0.3s, transform 0.1s;
    }

    button:hover { background: var(--accent-hover); }
    button:active { transform: scale(0.95); }

    .dashboard {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 1.5rem;
      width: 100%;
      max-width: 900px;
      opacity: 0.5;
      pointer-events: none;
      transition: opacity 0.3s;
    }

    .dashboard.active {
      opacity: 1;
      pointer-events: auto;
    }

    .card {
      background: var(--card-bg);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid var(--card-border);
      border-radius: 20px;
      padding: 2rem;
      display: flex;
      flex-direction: column;
      align-items: center;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
      transition: transform 0.3s, box-shadow 0.3s;
    }

    .card:hover {
      transform: translateY(-5px);
      box-shadow: 0 12px 40px rgba(0, 0, 0, 0.4);
    }

    .icon {
      font-size: 3rem;
      margin-bottom: 1rem;
    }

    .pulse {
      animation: pulse-animation 1s infinite;
      display: inline-block;
    }

    @keyframes pulse-animation {
      0% { transform: scale(1); }
      50% { transform: scale(1.2); }
      100% { transform: scale(1); }
    }

    .value {
      font-size: 2.5rem;
      font-weight: 700;
      margin-bottom: 0.5rem;
    }

    .label {
      color: var(--text-muted);
      font-size: 1rem;
      text-transform: uppercase;
      letter-spacing: 1px;
    }

    .status-dot {
      width: 10px;
      height: 10px;
      background-color: var(--danger);
      border-radius: 50%;
      display: inline-block;
      margin-right: 8px;
    }

    .status-dot.connected {
      background-color: var(--success);
      box-shadow: 0 0 10px var(--success);
    }

    .card.critical {
      border-color: var(--danger);
      box-shadow: 0 0 20px rgba(239, 68, 68, 0.4);
      background: rgba(239, 68, 68, 0.1);
      animation: pulse-danger 1s infinite alternate;
    }

    @keyframes pulse-danger {
      from { box-shadow: 0 0 10px rgba(239, 68, 68, 0.4); }
      to { box-shadow: 0 0 25px rgba(239, 68, 68, 0.8); }
    }
    
    .card.critical .value {
      color: var(--danger);
    }

    .alert-message {
      color: var(--danger);
      font-size: 0.9rem;
      font-weight: bold;
      margin-top: 0.8rem;
      display: none;
      text-transform: uppercase;
    }
    
    .card.critical .alert-message {
      display: block;
    }

  </style>
</head>
<body>

  <header>
    <h1>Smart Pet Dashboard</h1>
    <p><span id="statusDot" class="status-dot"></span><span id="statusText">Aguardando Conexão...</span></p>
  </header>

  <!-- Container de configuração removido, IP configurado automaticamente -->

  <div class="dashboard" id="dashboard">
    
    <div class="card" id="cardTemp">
      <div class="icon">🌡️</div>
      <div class="value"><span id="tempValue">--</span>°C</div>
      <div class="label">Temperatura</div>
      <div class="alert-message">⚠️ Risco Crítico</div>
    </div>

    <div class="card" id="cardBpm">
      <div class="icon pulse">❤️</div>
      <div class="value"><span id="bpmValue">--</span> bpm</div>
      <div class="label">Batimento</div>
      <div class="alert-message">⚠️ Risco Crítico</div>
    </div>

    <div class="card">
      <div class="icon">📍</div>
      <div class="value" style="font-size: 1.2rem; margin: 1rem 0;"><span id="gpsValue">-- / --</span></div>
      <div class="label">Localização (GPS)</div>
    </div>

  </div>

  <script>
    let fetchInterval;
    const collarIP = '127.0.0.2:9090';

    function initDashboard() {
      document.getElementById('dashboard').classList.add('active');
      document.getElementById('statusText').innerText = "Conectando ao ESP32 Servidor...";
      
      if (fetchInterval) clearInterval(fetchInterval);
      fetchData();
      fetchInterval = setInterval(fetchData, 2000);
    }
    
    // Inicia automaticamente quando a página carregar
    window.onload = initDashboard;

    async function fetchData() {
      try {
        const response = await fetch(`http://${collarIP}/api/data`);
        if (!response.ok) throw new Error("Erro na rede");
        
        const data = await response.json();
        
        document.getElementById('tempValue').innerText = data.temperatura.toFixed(1);
        document.getElementById('bpmValue').innerText = data.bpm;
        document.getElementById('gpsValue').innerText = `${data.lat.toFixed(4)}, ${data.lng.toFixed(4)}`;
        
        // Lógica de alerta crítico (mesmos limites do servidor)
        const isCriticalTemp = (data.temperatura < 36 || data.temperatura > 40);
        const isCriticalBPM = (data.bpm < 60 || data.bpm > 160);

        if (isCriticalTemp) {
          document.getElementById('cardTemp').classList.add('critical');
        } else {
          document.getElementById('cardTemp').classList.remove('critical');
        }

        if (isCriticalBPM) {
          document.getElementById('cardBpm').classList.add('critical');
        } else {
          document.getElementById('cardBpm').classList.remove('critical');
        }
        
        document.getElementById('statusDot').classList.add('connected');
        document.getElementById('statusText').innerText = "Conectado e recebendo dados em tempo real";

      } catch (error) {
        console.error("Erro ao buscar dados:", error);
        document.getElementById('statusDot').classList.remove('connected');
        document.getElementById('statusText').innerText = "Erro de conexão! Verifique o IP.";
      }
    }
  </script>

</body>
</html>
  )=====";
  
  server.send(200, "text/html", html);
}

void setup(void) {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Conectando ao WiFi (Dashboard/Cliente) ");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.println("==============================================");
  Serial.println("ABRA ESTE IP NO SEU NAVEGADOR PARA VER O DASHBOARD:");
  Serial.println("URL: http://127.0.0.1:9090");
  Serial.println("==============================================");

  server.on("/", HTTP_GET, sendHtml);

  server.begin();
  Serial.println("Servidor Web (Dashboard) iniciado!");
}

void loop(void) {
  server.handleClient();
  delay(2);
}
