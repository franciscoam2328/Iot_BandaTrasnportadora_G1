#ifndef CONFIG_H
#define CONFIG_H

// --- WiFi Credentials ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Supabase Credentials ---
const char* supabase_url = "https://evorufjkfqmqmjhdpxeo.supabase.co";
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImV2b3J1ZmprZnFtcW1qaGRweGVvIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjM3Njc1NTgsImV4cCI6MjA3OTM0MzU1OH0.ewTwn9B7cj_5ibr12UiMxxhd0sM5CTRfCF4IiepCJrI";

// --- Pin Definitions ---
// Sensor de Color (TCS230/TCS3200)
#define S0 18
#define S1 19
#define S2 21
#define S3 22
#define SENSOR_OUT 23

// Sensor de Objetos (FC-51)
#define SENSOR_OBJETO_PIN 32

// Motor DC (Puente H)
#define MOTOR_IN1 26
#define MOTOR_IN2 27

#endif
