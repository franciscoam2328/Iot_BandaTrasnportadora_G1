#include <WiFi.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include "config.h"

// --- Objetos Globales ---


// --- Variables de Estado ---
bool sistemaEncendido = false;
String turnoActual = "turno1";
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 2000; // Consultar estado cada 2s

// --- Prototipos de Funciones ---
void connectWiFi();
void checkSystemStatus();
void controlMotor(bool on);
void readColorAndAct();
String detectColor();
void sendDataToSupabase(String color);
void moveServo(String color);

void setup() {
  Serial.begin(115200);

  // Configurar Pines
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  pinMode(SENSOR_OBJETO_PIN, INPUT);
  
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);

  // Configurar Escala de Frecuencia del Sensor (20%)
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);



  connectWiFi();
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Consultar estado del sistema periódicamente
  if (millis() - lastCheckTime > checkInterval) {
    checkSystemStatus();
    lastCheckTime = millis();
  }

  if (sistemaEncendido) {
    controlMotor(true);
    // Leer sensor de objetos (FC-51 suele ser LOW cuando detecta)
    if (digitalRead(SENSOR_OBJETO_PIN) == LOW) {
       readColorAndAct();
       // Pequeño debounce o espera para no leer el mismo objeto múltiples veces
       delay(1000); 
    }
  } else {
    controlMotor(false);
  }
  
  delay(100);
}

void connectWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
}

void checkSystemStatus() {
  HTTPClient http;
  String url = String(supabase_url) + "/rest/v1/sistema?select=encendido,turno_actual&limit=1";
  
  http.begin(url);
  http.addHeader("apikey", supabase_key);
  http.addHeader("Authorization", String("Bearer ") + supabase_key);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    // Parsear JSON (Array con 1 objeto)
    // Ejemplo: [{"encendido":true, "turno_actual":"turno1"}]
    // Nota: Usar ArduinoJson para parsear robustamente
    // Por simplicidad aquí, asumimos que funciona o usamos una librería
    
    // TODO: Implementar parseo real con ArduinoJson
    // Serial.println(payload);
  } else {
    Serial.println("Error en HTTP GET");
  }
  http.end();
}

void controlMotor(bool on) {
  if (on) {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
  } else {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
  }
}

void readColorAndAct() {
  String color = detectColor();
  
  if (color != "UNKNOWN") {
    Serial.println("Color Detectado: " + color);
    
    // 1. Detener motor momentáneamente para precisión (opcional)
    controlMotor(false);
    delay(500);
    
    // 2. Enviar a Supabase
    sendDataToSupabase(color);
    
    // 3. Reanudar motor
    controlMotor(true);
  }
}

String detectColor() {
  // Leer Rojo
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  int redPW = pulseIn(SENSOR_OUT, LOW);
  delay(10);
  
  // Leer Verde
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  int greenPW = pulseIn(SENSOR_OUT, LOW);
  delay(10);
  
  // Leer Azul
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  int bluePW = pulseIn(SENSOR_OUT, LOW);
  delay(10);

  // Calibración simple (Valores ejemplo, el usuario debe calibrar)
  // Asumimos que menor pulseWidth = mayor intensidad
  
  if (redPW < greenPW && redPW < bluePW && redPW < 100) {
    return "ROJO";
  }
  if (greenPW < redPW && greenPW < bluePW && greenPW < 100) {
    return "VERDE";
  }
  if (bluePW < redPW && bluePW < greenPW && bluePW < 100) {
    return "AZUL";
  }
  
  return "UNKNOWN";
}



void sendDataToSupabase(String color) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // 1. Insertar en tabla 'lecturas'
    String urlLecturas = String(supabase_url) + "/rest/v1/lecturas";
    http.begin(urlLecturas);
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_key);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");
    
    String jsonLectura = "{\"color\":\"" + color + "\", \"turno\":\"" + turnoActual + "\"}";
    int httpCode = http.POST(jsonLectura);
    http.end();
    
    // 2. Llamar RPC 'incrementar_contador'
    String urlRpc = String(supabase_url) + "/rest/v1/rpc/incrementar_contador";
    http.begin(urlRpc);
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_key);
    http.addHeader("Content-Type", "application/json");
    
    String jsonRpc = "{\"p_turno\":\"" + turnoActual + "\", \"p_color\":\"" + color + "\"}";
    http.POST(jsonRpc);
    http.end();
  }
}
