#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

// =========================================================
// CONFIGURACIÓN WIFI
// =========================================================
const char* ssid     = "ARONI-WIFI6";
const char* password = "Aroni972310";

// =========================================================
// CONFIGURACIÓN SUPABASE
// =========================================================
const char* supabase_url = "https://evorufjkfqmqmjhdpxeo.supabase.co";
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImV2b3J1ZmprZnFtcW1qaGRweGVvIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjM3Njc1NTgsImV4cCI6MjA3OTM0MzU1OH0.ewTwn9B7cj_5ibr12UiMxxhd0sM5CTRfCF4IiepCJrI";

// =========================================================
// HARDWARE
// =========================================================
// Servos
Servo servoRojo;
Servo servoVerde;
#define SERVO_ROJO_PIN 15
#define SERVO_VERDE_PIN 18

// Motor
#define MOTOR_IN1 26
#define MOTOR_IN2 27

// Sensor TCS230
#define S0 4
#define S1 16
#define S2 21
#define S3 22
#define SENSOR_OUT 23
#define FC51_PIN 13 // Sensor de obstaculos IR

// =========================================================
// VARIABLES GLOBALES
// =========================================================
bool sistemaEncendido = true; // Asumimos encendido por defecto hasta leer
String turnoActual = "turno1";
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 5000; // Revisar Supabase cada 5 segundos

// =========================================================
// FUNCIONES AUXILIARES
// =========================================================

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
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Consultamos estado y turno en una sola llamada para ser eficientes
    String url = String(supabase_url) + "/rest/v1/sistema?select=encendido,turno_actual&limit=1";
    
    http.begin(url);
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_key);
    
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      // Parseo simple (JSON manual)
      if (payload.indexOf("\"encendido\":true") > 0) sistemaEncendido = true;
      else if (payload.indexOf("\"encendido\":false") > 0) sistemaEncendido = false;
      
      if (payload.indexOf("turno1") > 0) turnoActual = "turno1";
      else if (payload.indexOf("turno2") > 0) turnoActual = "turno2";
      else if (payload.indexOf("turno3") > 0) turnoActual = "turno3";
    }
    http.end();
  }
}

void enviarLectura(String color) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // 1. Insertar en tabla lecturas
    String url = String(supabase_url) + "/rest/v1/lecturas";
    http.begin(url);
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_key);
    http.addHeader("Content-Type", "application/json");
    
    String json = "{\"color\":\"" + color + "\", \"turno\":\"" + turnoActual + "\"}";
    http.POST(json);
    http.end();
    
    // 2. Llamar RPC para contadores
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

// =========================================================
// SETUP
// =========================================================
void setup() {
  Serial.begin(115200);
  
  // Configurar Pines
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  pinMode(FC51_PIN, INPUT);
  
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);

  // Escala de Frecuencia 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Servos SG90 (500-2400us)
  servoRojo.setPeriodHertz(50);
  servoVerde.setPeriodHertz(50);
  servoRojo.attach(SERVO_ROJO_PIN, 500, 2400);
  servoVerde.attach(SERVO_VERDE_PIN, 500, 2400);
  servoRojo.write(0);
  servoVerde.write(0);

  // Test de Servos al inicio (Para verificar que funcionan)
  Serial.println("Probando Servos...");
  servoRojo.write(90);
  servoVerde.write(90);
  delay(500);
  servoRojo.write(0);
  servoVerde.write(0);
  Serial.println("Test de Servos completado.");

  connectWiFi();
}

// =========================================================
// LOOP
// =========================================================
void loop() {
  // 1. Verificar WiFi
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  // 2. Actualizar estado (NO BLOQUEANTE, cada 2 seg)
  if (millis() - lastCheckTime > checkInterval) {
    checkSystemStatus();
    lastCheckTime = millis();
  }

  // 3. Lógica Principal
  if (!sistemaEncendido) {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    delay(100);
    return;
  }

  // Motor Encendido
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);

  // 4. Verificar si hay objeto (FC-51 activo en LOW)
  int irValue = digitalRead(FC51_PIN);
  if (irValue == HIGH) {
    // Si NO hay objeto, no leemos color ni movemos servos
    // Imprimimos un punto "." cada tanto para saber que está vivo y esperando
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
       Serial.println("Esperando objeto (Sensor IR = HIGH)...");
       lastPrint = millis();
    }
    return; 
  }
  
  Serial.println("¡Objeto Detectado! Leyendo color...");

  // 5. Leer Sensor (Timeout corto para no frenar motor)
  unsigned long timeout = 15000; // 15ms

  // Leemos ROJO
  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  delay(20); // Estabilizar
  int rojo = pulseIn(SENSOR_OUT, LOW, timeout);

  // Leemos VERDE
  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  delay(20); // Estabilizar
  int verde = pulseIn(SENSOR_OUT, LOW, timeout);

  // Leemos AZUL
  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  delay(20); // Estabilizar
  int azul = pulseIn(SENSOR_OUT, LOW, timeout);

  // Si hubo timeout, ignorar
  if (rojo == 0 || verde == 0 || azul == 0) return;

  // Debug
  Serial.printf("R:%d G:%d B:%d\n", rojo, verde, azul);

  // 6. Clasificación (Mejorada: Lógica Relativa)
  // En lugar de exigir que los otros colores sean > 1000, solo pedimos que el color deseado sea el "ganador" (el valor más bajo).
  
  // ROJO: El valor Rojo es el menor de los tres y está por debajo de un umbral razonable
  if (rojo < verde && rojo < azul && rojo < 1500) {
    Serial.println("🔴 ROJO DETECTADO -> Moviendo Servo Rojo");
    servoRojo.write(90);
    enviarLectura("ROJO");
    delay(3000); // Esperar 3 segundos
    servoRojo.write(0);
  }
  // VERDE: El valor Verde es el menor de los tres
  else if (verde < rojo && verde < azul && verde < 3000) {
    Serial.println("🟢 VERDE DETECTADO -> Moviendo Servo Verde");
    servoVerde.write(90);
    enviarLectura("VERDE");
    delay(3000); // Esperar 3 segundos
    servoVerde.write(0);
  }
  // AZUL: El valor Azul es el menor de los tres
  else if (azul < rojo && azul < verde && azul < 3000) {
    Serial.println("🔵 AZUL DETECTADO -> Paso libre");
    enviarLectura("AZUL");
    delay(2000); 
  }
}
