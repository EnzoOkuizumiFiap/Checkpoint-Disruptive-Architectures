#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 18
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_CRITICO 26 // LED Vermelho
#define LED_OK 27      // LED Verde

// LCD no I2C (endereço padrão 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

WebServer server(80);

// Variáveis dos sensores da coleira
float temperatura = 0;
int bpm = 110;

// Coordenada GPS (Ex: São Paulo)
float lat = -23.550520;
float lng = -46.633308;

unsigned long lastUpdate = 0;

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

void setup(void) {
  Serial.begin(115200);

  // Configuração dos pinos e sensores
  pinMode(LED_CRITICO, OUTPUT);
  pinMode(LED_OK, OUTPUT);
  
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
  Serial.println("COPIE ESTE IP PARA USAR NO SEU DASHBOARD:");
  Serial.println("IP da Coleira: 127.0.0.2:9090");
  Serial.println("==============================================");

  // Atualiza o LCD com o IP
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Conectado!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(2000);

  // Endpoint da API
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
  
  // Simula e atualiza os sensores a cada 5 segundos
  if (millis() - lastUpdate > 5000) {
    lastUpdate = millis();
    
    // Lê a temperatura real do DHT22
    float t = dht.readTemperature();
    if (!isnan(t)) {
      temperatura = t;
    }

    // Simulação mais realista do batimento (BPM)
    int evento = random(0, 100);
    if (evento < 10) {
      // 10% de chance de pico de agitação (ex: animal correndo/latindo)
      bpm += random(20, 55);
    } else if (evento < 40) {
      // 30% de chance de relaxamento profundo (ex: animal dormindo)
      bpm -= random(15, 30);
    } else {
      // 60% de chance de variação normal (respiração)
      bpm += random(-3, 4);
    }
    
    // Tendência natural do corpo de voltar ao batimento em repouso (~110)
    if (bpm > 120) bpm -= random(1, 4);
    if (bpm < 90) bpm += random(1, 4);

    // Limites absolutos físicos para a simulação não quebrar
    if (bpm < 50) bpm = 50;   
    if (bpm > 180) bpm = 180; 
    
    // Variação minúscula no GPS simulando o pet se movendo
    lat += (random(-5, 6) / 100000.0);
    lng += (random(-5, 6) / 100000.0);

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

    lcd.clear();

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
      // Se tiver ALGUM problema, mostra o status de cada um em uma linha

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