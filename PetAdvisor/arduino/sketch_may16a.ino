/*
 * Полная система: 2 тензодатчика (HX711) + передача данных по Bluetooth (HC-05)
 * ВЫВОД В СТОЛБЕЦ
 */

#include "HX711.h"
#include <SoftwareSerial.h>

// ========== НАСТРОЙКИ ТЕНЗОДАТЧИКОВ ==========
const int LOADCELL1_DOUT_PIN = A1;
const int LOADCELL1_SCK_PIN  = A0;
const int LOADCELL2_DOUT_PIN = A2;
const int LOADCELL2_SCK_PIN  = A3;

float CALIBRATION_FACTOR1 = 176;
float CALIBRATION_FACTOR2 = 190.0;

// ========== НАСТРОЙКИ BLUETOOTH ==========
SoftwareSerial BTSerial(10, 11);

HX711 scale1;
HX711 scale2;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 500;

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  
  Serial.println("=== СИСТЕМА ЗАПУЩЕНА ===");
  Serial.println();
  
  // Инициализация датчиков
  scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
  scale1.set_gain(128);
  scale1.set_scale(CALIBRATION_FACTOR1);
  scale1.tare();
  
  scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
  scale2.set_gain(128);
  scale2.set_scale(CALIBRATION_FACTOR2);
  scale2.tare();
  
  delay(500);
  
  Serial.println("Датчик 1: " + String(scale1.is_ready() ? "OK" : "ОШИБКА"));
  Serial.println("Датчик 2: " + String(scale2.is_ready() ? "OK" : "ОШИБКА"));
  Serial.println();
  Serial.println("=== КОМАНДЫ ===");
  Serial.println("t1 - обнулить датчик 1");
  Serial.println("t2 - обнулить датчик 2");
  Serial.println("c1 - калибровка датчика 1");
  Serial.println("c2 - калибровка датчика 2");
  Serial.println("=================");
  Serial.println();
  
  BTSerial.println("System ready");
}

void loop() {
  // Чтение данных с датчиков
  float weight1 = 0;
  float weight2 = 0;
  bool error1 = false;
  bool error2 = false;
  
  if (scale1.is_ready()) {
    weight1 = scale1.get_units(10);
  } else {
    error1 = true;
  }
  
  if (scale2.is_ready()) {
    weight2 = scale2.get_units(10);
  } else {
    error2 = true;
  }
  
  // ========== ВЫВОД В СТОЛБЕЦ ==========
  if (!error1 && !error2) {
    float total = weight1 + weight2;
    
    // Очищаем экран (опционально - раскомментировать если нужно)
    // Serial.print("\033[2J\033[H");  // Для терминалов ANSI
    
    Serial.println("=========================");
    Serial.print("Датчик 1: ");
    Serial.print(weight1, 1);
    Serial.println(" г");
    
    Serial.print("Датчик 2: ");
    Serial.print(weight2, 1);
    Serial.println(" г");
    
    Serial.print("СУММА:   ");
    Serial.print(total, 1);
    Serial.println(" г");
    
    Serial.print("Время:   ");
    Serial.print(millis() / 1000);
    Serial.println(" с");
    Serial.println("=========================");
    Serial.println();
    
  } else {
    if (error1) Serial.println("ОШИБКА: Датчик 1 не отвечает!");
    if (error2) Serial.println("ОШИБКА: Датчик 2 не отвечает!");
    Serial.println();
  }
  
  // Отправка по Bluetooth
  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;
    
    if (!error1 && !error2) {
      float total = weight1 + weight2;
      BTSerial.print("DATA:");
      BTSerial.print(weight1, 1);
      BTSerial.print(",");
      BTSerial.print(weight2, 1);
      BTSerial.print(",");
      BTSerial.println(total, 1);
    } else {
      BTSerial.println("ERROR");
    }
  }
  
  // Обработка команд из Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "t1" || command == "T1") {
      scale1.tare();
      Serial.println(">>> ДАТЧИК 1 ОБНУЛЁН <<<");
      Serial.println();
      BTSerial.println("Sensor 1 tared");
    }
    else if (command == "t2" || command == "T2") {
      scale2.tare();
      Serial.println(">>> ДАТЧИК 2 ОБНУЛЁН <<<");
      Serial.println();
      BTSerial.println("Sensor 2 tared");
    }
    else if (command == "c1" || command == "C1") {
      calibrateScale(scale1, 1);
    }
    else if (command == "c2" || command == "C2") {
      calibrateScale(scale2, 2);
    }
  }
  
  // Команда по Bluetooth
  if (BTSerial.available()) {
    char btCmd = BTSerial.read();
    if (btCmd == 't' || btCmd == 'T') {
      scale1.tare();
      scale2.tare();
      BTSerial.println("Both sensors tared");
      Serial.println(">>> ОБА ДАТЧИКА ОБНУЛЕНЫ ПО BLUETOOTH <<<");
      Serial.println();
    }
  }
  
  delay(200);
}

// Функция калибровки
void calibrateScale(HX711 &scale, int sensorNum) {
  Serial.println();
  Serial.println("=== КАЛИБРОВКА ДАТЧИКА " + String(sensorNum) + " ===");
  Serial.println("Уберите груз с датчика");
  Serial.println("Нажмите любую клавишу для обнуления...");
  
  while (!Serial.available()) { }
  while (Serial.available()) Serial.read();
  
  scale.tare();
  Serial.println("Ноль установлен.");
  
  Serial.print("Положите эталонный груз на датчик ");
  Serial.print(sensorNum);
  Serial.print(". Введите вес в граммах: ");
  
  while (!Serial.available()) { }
  float knownWeight = Serial.parseFloat();
  
  if (knownWeight <= 0) {
    Serial.println("ОШИБКА: Вес должен быть положительным!");
    return;
  }
  
  long rawReading = scale.read_average(20);
  Serial.print("Сырые данные: ");
  Serial.println(rawReading);
  
  float newFactor = rawReading / knownWeight;
  Serial.println();
  Serial.println(">>> НОВЫЙ КАЛИБРОВОЧНЫЙ КОЭФФИЦИЕНТ: " + String(newFactor, 4));
  Serial.println("Замените CALIBRATION_FACTOR" + String(sensorNum) + " на это значение");
  Serial.println();
  
  scale.set_scale(newFactor);
  Serial.print("Проверка: вес = ");
  Serial.print(scale.get_units(10), 1);
  Serial.println(" г (должен быть " + String(knownWeight, 1) + " г)");
  Serial.println();
}
