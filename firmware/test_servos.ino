#include <ESP32Servo.h>

Servo servoRojo;
Servo servoVerde;

// Pines definidos por el usuario
#define SERVO_ROJO_PIN 15
#define SERVO_VERDE_PIN 18

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Test de Servos...");

  // Configuración para SG90 (500-2400us)
  servoRojo.setPeriodHertz(50);
  servoVerde.setPeriodHertz(50);

  servoRojo.attach(SERVO_ROJO_PIN, 500, 2400);
  servoVerde.attach(SERVO_VERDE_PIN, 500, 2400);

  // Posición inicial
  servoRojo.write(0);
  servoVerde.write(0);
  delay(1000);
}

void loop() {
  Serial.println("Moviendo a 45 grados...");
  servoRojo.write(45);
  servoVerde.write(45);
  delay(1000);

  Serial.println("Moviendo a 90 grados...");
  servoRojo.write(90);
  servoVerde.write(90);
  delay(1000);

  Serial.println("Regresando a 0 grados...");
  servoRojo.write(0);
  servoVerde.write(0);
  delay(1000);
}
