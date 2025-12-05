#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Asegúrate de instalar la librería "ArduinoJson" desde el Gestor de Librerías

// ==========================================
// CONFIGURACIÓN
// ==========================================

// --- Credenciales WiFi ---
// REEMPLAZA CON TU RED Y CONTRASEÑA REALES
const char* ssid = "TU_RED_WIFI"; 
const char* password = "TU_CONTRASEÑA_WIFI";

// --- Credenciales Supabase ---
const char* supabase_url = "https://evorufjkfqmqmjhdpxeo.supabase.co";
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImV2b3J1ZmprZnFtcW1qaGRweGVvIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjM3Njc1NTgsImV4cCI6MjA3OTM0MzU1OH0.ewTwn9B7cj_5ibr12UiMxxhd0sM5CTRfCF4IiepCJrI";

// --- Definición de Pines ---
// Sensor de Color (TCS230/TCS3200)
#define S0 18
#define S1 19
#define S2 21
#define S3 22
#define SENSOR_OUT 23

// Sensor de Objetos (FC-51)
#define SENSOR_OBJETO_PIN 32

// Motor DC (Puente H - L9110S o similar)
#define MOTOR_IN1 26
#define MOTOR_IN2 27

// ==========================================
// VARIABLES GLOBALES
// ==========================================

bool sistemaEncendido = false;
String turnoActual = "turno1";
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 2000; // Consultar estado cada 2s

// ==========================================
// PROTOTIPOS
// ==========================================
void connectWiFi();
void checkSystemStatus();
void controlMotor(bool on);
void readColorAndAct();
String detectColor();
void sendDataToSupabase(String color);

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  // Configurar Pines
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  pinMode(SENSOR_OBJETO_PIN, INPUT);
  
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);

  // Configurar Escala de Frecuencia del Sensor de Color (20%)
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Iniciar WiFi
  connectWiFi();
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
  // 1. Verificar conexión WiFi y reconectar si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // 2. Consultar estado del sistema periódicamente (Polling)
  if (millis() - lastCheckTime > checkInterval) {
    checkSystemStatus();
    lastCheckTime = millis();
  }

  // 3. Lógica de Control
  if (sistemaEncendido) {
    controlMotor(true);
    
    // Leer sensor de objetos (FC-51 suele dar LOW cuando detecta obstáculo)
    if (digitalRead(SENSOR_OBJETO_PIN) == LOW) {
       Serial.println("Objeto detectado! Iniciando lectura de color...");
       
       // Pequeña pausa para estabilizar objeto frente al sensor de color
       delay(500); 
       
       readColorAndAct();
       
       // Evitar múltiples lecturas del mismo objeto
       delay(1000); 
    }
  } else {
    controlMotor(false);
  }
  
  delay(100); // Pequeño delay para no saturar el loop
}

// ==========================================
// FUNCIONES AUXILIARES
// ==========================================

void connectWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFallo al conectar WiFi.");
  }
}

void checkSystemStatus() {
  HTTPClient http;
  // Consultamos solo los campos necesarios
  String url = String(supabase_url) + "/rest/v1/sistema?select=encendido,turno_actual&limit=1";
  
  http.begin(url);
  http.addHeader("apikey", supabase_key);
  http.addHeader("Authorization", String("Bearer ") + supabase_key);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    
    // Parsear JSON (Array con 1 objeto)
    // Ejemplo: [{"encendido":true, "turno_actual":"turno1"}]
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // El resultado es un array, accedemos al primer elemento [0]
      JsonObject root = doc[0];
      
      bool nuevoEstado = root["encendido"];
      const char* nuevoTurno = root["turno_actual"];
      
      sistemaEncendido = nuevoEstado;
      if (nuevoTurno) {
        turnoActual = String(nuevoTurno);
      }
      
      // Debug opcional
      // Serial.print("Estado Remoto: "); Serial.println(sistemaEncendido ? "ON" : "OFF");
    } else {
      Serial.print("Error JSON: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("Error HTTP GET: ");
    Serial.println(httpCode);
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
  // 1. Esperar un poco para que el objeto llegue justo debajo del sensor de color
  // (Ajusta este valor según la velocidad de tu motor y la distancia entre sensores)
  delay(400); 
  
  // 2. Detectar Color (Promediando lecturas para mayor precisión)
  String color = detectColor();
  
  if (color != "UNKNOWN") {
    Serial.println("Color Confirmado: " + color);
    sendDataToSupabase(color);
  } else {
    Serial.println("Color Desconocido / Lectura inestable");
  }
  
  // Evitar re-lecturas inmediatas del mismo objeto
  delay(500);
}

String detectColor() {
  long totalR = 0, totalG = 0, totalB = 0;
  int muestras = 5; // Tomar 5 muestras para promediar

  for(int i=0; i<muestras; i++) {
    // Leer Rojo
    digitalWrite(S2, LOW); digitalWrite(S3, LOW);
    totalR += pulseIn(SENSOR_OUT, LOW);
    
    // Leer Verde
    digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
    totalG += pulseIn(SENSOR_OUT, LOW);
    
    // Leer Azul
    digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
    totalB += pulseIn(SENSOR_OUT, LOW);
    
    delay(5); // Pequeña pausa entre muestras
  }

  // Calcular promedios
  int redPW = totalR / muestras;
  int greenPW = totalG / muestras;
  int bluePW = totalB / muestras;

  Serial.print("Promedio -> R:"); Serial.print(redPW);
  Serial.print(" G:"); Serial.print(greenPW);
  Serial.print(" B:"); Serial.println(bluePW);

  // --- LÓGICA DE DECISIÓN ---
  // Ajusta estos valores según tus pruebas. 
  // Generalmente: El valor más bajo es el color detectado.
  
  if (redPW < greenPW && redPW < bluePW && redPW < 400) {
    return "ROJO";
  }
  if (greenPW < redPW && greenPW < bluePW && greenPW < 400) {
    return "VERDE";
  }
  if (bluePW < redPW && bluePW < greenPW && bluePW < 400) {
    return "AZUL";
  }
  
  return "UNKNOWN";
}

void sendDataToSupabase(String color) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // A. Insertar en tabla 'lecturas' (Historial)
    String urlLecturas = String(supabase_url) + "/rest/v1/lecturas";
    http.begin(urlLecturas);
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_key);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");
    
    String jsonLectura = "{\"color\":\"" + color + "\", \"turno\":\"" + turnoActual + "\"}";
    int httpCode = http.POST(jsonLectura);
    http.end();
    
    if (httpCode == 201) {
      Serial.println("Lectura guardada en DB.");
    } else {
      Serial.print("Error guardando lectura: "); Serial.println(httpCode);
    }
    
    // B. Llamar RPC 'incrementar_contador' (Contadores en tiempo real)
    String urlRpc = String(supabase_url) + "/rest/v1/rpc/incrementar_contador";
    http.begin(urlRpc);
    http.addHeader("apikey", supabase_key);
    http.addHeader("Authorization", String("Bearer ") + supabase_key);
    http.addHeader("Content-Type", "application/json");
    
    String jsonRpc = "{\"p_turno\":\"" + turnoActual + "\", \"p_color\":\"" + color + "\"}";
    int rpcCode = http.POST(jsonRpc);
    http.end();
    
    if (rpcCode == 200 || rpcCode == 204) {
      Serial.println("Contador incrementado.");
    } else {
      Serial.print("Error RPC: "); Serial.println(rpcCode);
    }
  }
}
