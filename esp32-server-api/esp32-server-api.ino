#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS++.h>

#define GPS_BAUDRATE 9600
#define PULSE_PIN 35
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // Usa a UART1

#define DHTPIN 18
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_CRITICO 26 // LED Vermelho
#define LED_OK 27      // LED Verde

#define BUTTON_PIN 14
bool showGPS = false;
bool lastButtonState = HIGH;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

WebServer server(80);

// Variáveis dos sensores da coleira
float temperatura = 0;
int bpm = 0;

// Coordenada GPS (Ex: São Paulo)
float lat = -23.550520;
float lng = -46.633308;

unsigned long lastUpdate = 0;

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
      <div class="alert-message">Risco Crítico</div>
    </div>

    <div class="card" id="cardBpm">
      <div class="icon pulse">❤️</div>
      <div class="value"><span id="bpmValue">--</span> bpm</div>
      <div class="label">Batimento</div>
      <div class="alert-message">Risco Crítico</div>
    </div>

    <div class="card">
      <div class="icon">📍</div>
      <div class="value" style="font-size: 1.2rem; margin: 1rem 0;"><span id="gpsValue">-- / --</span></div>
      <div class="label">Localização (GPS)</div>
    </div>

  </div>

  <script>
    let fetchInterval;

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
        // Fetch relativo! Como a página e a API estão no mesmo ESP32, não precisamos fixar o IP.
        const response = await fetch('/api/data');
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

void sendJsonStatus() {
  // CORS: Permite que outras páginas web requisitem esses dados
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  String response = "{";
  response += "\"temperatura\":" + String(temperatura, 1) + ",";
  response += "\"bpm\":" + String(bpm) + ",";
  response += "\"lat\":" + String(lat, 6) + ",";
  response += "\"lng\":" + String(lng, 6);
  response += "}";

  server.send(200, "application/json", response);
}

void updateLCD() {
  bool isCriticalTemp = (temperatura < 36 || temperatura > 40);
  bool isCriticalBPM = (bpm < 60 || bpm > 160);

  lcd.clear();
  
  if (showGPS) {
    lcd.setCursor(0, 0);
    lcd.print("Lat:");
    lcd.print(lat, 6);
    lcd.setCursor(0, 1);
    lcd.print("Lng:");
    lcd.print(lng, 6);
  } else {
    // Se estiver tudo bem, mostra a mensagem de OK e os dados resumidos
    if (!isCriticalTemp && !isCriticalBPM) {
      lcd.setCursor(0, 0);
      lcd.print("Tudo OK!");
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(temperatura, 1);
      lcd.print("C  BPM:");
      lcd.print(bpm);
    } else {
      // Linha 0 (Temperatura)
      lcd.setCursor(0, 0);
      lcd.print(isCriticalTemp ? "ALERTA T: " : "Temp OK: ");
      lcd.print(temperatura, 1);
      lcd.print("C");
      
      // Linha 1 (BPM)
      lcd.setCursor(0, 1);
      lcd.print(isCriticalBPM ? "ALERTA BPM: " : "BPM OK: ");
      lcd.print(bpm);
    }
  }
}

void setup(void) {
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUDRATE, SERIAL_8N1, 16, 17); // Inicia a serial do GPS RX=16 e TX=17
  
  // Configuração dos pinos e sensores
  pinMode(LED_CRITICO, OUTPUT);
  pinMode(LED_OK, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  dht.begin();
  
  // Configuração do LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print("Conectando ao WiFi (Servidor/Coleira) ");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
  Serial.println("==============================================");
  Serial.println("IPS:");
  Serial.println("IP da Coleira: 127.0.0.1:9090");
  Serial.println("IP do Dashboard: http://127.0.0.2:9090");
  Serial.println("==============================================");

  // Atualiza o LCD com o IP
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Conectado!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(2000);

  // Endpoints do Servidor
  server.on("/", HTTP_GET, sendHtml); // Rota principal serve a página do Dashboard
  
  server.on("/api/data", HTTP_GET, []() {
    sendJsonStatus();
  });

  // Rota OPTIONS para CORS
  server.on("/api/data", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.send(204);
  });

  server.onNotFound([]() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(404, "application/json", "{\"error\":\"Rota nao encontrada\"}");
  });

  server.begin();
  Serial.println("Servidor da Coleira (API) iniciado!");
}

void loop(void) {
  server.handleClient();
  
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  if (currentButtonState != lastButtonState) {
    delay(50); // Aguarda o bounce estabilizar
    currentButtonState = digitalRead(BUTTON_PIN); // Confirma o novo estado
    
    // Dispara a ação apenas na transição de solto (HIGH) para pressionado (LOW)
    if (currentButtonState == LOW && lastButtonState == HIGH) {
      showGPS = !showGPS;
      updateLCD();
    }
    lastButtonState = currentButtonState;
  }

  // Le os dados do GPS sempre que disponiveis
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        lat = gps.location.lat();
        lng = gps.location.lng();
      }
    }
  }

  // Atualiza os sensores a cada 5 segundos
  if (millis() - lastUpdate > 5000) {
    lastUpdate = millis();
    
    // Debug GPS
    Serial.print("Caracteres recebidos do GPS: ");
    Serial.println(gps.charsProcessed());
    Serial.print("GPS Satelites: ");
    Serial.println(gps.satellites.value());
    if (!gps.location.isValid()) {
      Serial.println("Aviso: GPS ainda nao encontrou sinal valido (usando valor inicial).");
    } else {
      Serial.println("GPS OK! Localizacao atualizada.");
    }
    
    // Lê a temperatura do DHT22
    temperatura = dht.readTemperature();

    // Leitura do valor analógico do sensor de pulso (simulado via potenciômetro)
    int16_t pulseValue = analogRead(PULSE_PIN);
    // Mapeia o valor bruto do potenciômetro (0 a 4095 no ESP32) para a faixa de BPM (40 a 200)
    bpm = map(pulseValue, 0, 4095, 40, 200);
    
    // Lógica de alerta crítico
    // Condições críticas: temperatura fora de 36 - 40 ou BPM fora de 60 - 160
    bool isCriticalTemp = (temperatura < 36 || temperatura > 40);
    bool isCriticalBPM = (bpm < 60 || bpm > 160);

    if (isCriticalTemp || isCriticalBPM) {
      digitalWrite(LED_CRITICO, HIGH);
      digitalWrite(LED_OK, LOW);
    } else {
      digitalWrite(LED_CRITICO, LOW);
      digitalWrite(LED_OK, HIGH);
    }

    updateLCD();
  }
}