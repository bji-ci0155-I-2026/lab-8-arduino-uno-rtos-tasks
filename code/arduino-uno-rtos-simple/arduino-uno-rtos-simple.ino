#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// --- Definición de Pines ---
const int CAR_RED_PIN = 12, CAR_YELLOW_PIN = 11, CAR_GREEN_PIN = 10;
const int PED_RED_PIN = 9, PED_GREEN_PIN = 8;
const int BUTTON_PIN = 2;

// --- Estados del Sistema ---
enum TrafficState {
  CAR_GREEN,      // Verde para vehículos, rojo para peatones
  CAR_YELLOW,     // Amarillo para vehículos (transición)
  PED_CROSSING,   // Verde para peatones (cruzan)
  PED_WARNING,    // Verde parpadeante para peatones (advertencia)
  COOLDOWN        // Verde vehículos de nuevo (enfriamiento de seguridad)
};

volatile TrafficState systemState = CAR_GREEN; // Estado inicial
volatile unsigned long requests = 0; // Contador de cruces solicitados

SemaphoreHandle_t xSemButton = NULL; // Semáforo para comunicación entre tareas

void controllerTask(void *pv); // Controlador de semáforo
void monitorTask(void *pv); // Monitor de semáforo
void buttonISR();

void setup() {
  Serial.begin(9600);
  
  pinMode(CAR_RED_PIN, OUTPUT);
  pinMode(CAR_YELLOW_PIN, OUTPUT);
  pinMode(CAR_GREEN_PIN, OUTPUT);
  pinMode(PED_RED_PIN, OUTPUT);
  pinMode(PED_GREEN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Estado Inicial
  digitalWrite(CAR_GREEN_PIN, HIGH);
  digitalWrite(PED_RED_PIN, HIGH);

  xSemButton = xSemaphoreCreateBinary();

  if (xSemButton != NULL) {
    // Tareas con stack balanceado y seguro (100 y 150 palabras)
    xTaskCreate(controllerTask, "Ctrl", 100, NULL, 2, NULL);
    xTaskCreate(monitorTask, "Mon", 150, NULL, 1, NULL);
    
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    Serial.println(F("RTOS Ready"));
  }
}

void loop() {}

// Rutina de Interrupción
void buttonISR() {
  BaseType_t woken = pdFALSE;
  // Funciona como productor de eventos
  xSemaphoreGiveFromISR(xSemButton, &woken);
  if (woken) {
    // Cede el control de la CPU a la tarea de mayor prioridad manualmente
    taskYIELD();
  }
}

// Tarea del Controlador (Prioridad 2)
void controllerTask(void *pv) {
  (void) pv;
  for (;;) {
    // Funciona como consumidor de eventos
    if (xSemaphoreTake(xSemButton, portMAX_DELAY) == pdTRUE) {
      Serial.println(F("\n[Crossing Requested]"));

      // 1. Autos de Verde a Amarillo
      systemState = CAR_YELLOW;
      digitalWrite(CAR_GREEN_PIN, LOW);
      digitalWrite(CAR_YELLOW_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(4000));

      // 2. Autos a Rojo y espera
      digitalWrite(CAR_YELLOW_PIN, LOW);
      digitalWrite(CAR_RED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(3000));

      // 3. Cruce peatonal activo (Verde Peatón)
      systemState = PED_CROSSING;
      digitalWrite(PED_RED_PIN, LOW);
      digitalWrite(PED_GREEN_PIN, HIGH);
      requests++;
      vTaskDelay(pdMS_TO_TICKS(7000));

      // 4. Parpadeo Verde Peatón (Advertencia)
      systemState = PED_WARNING;
      for (int i = 0; i < 8; i++) {
        digitalWrite(PED_GREEN_PIN, !digitalRead(PED_GREEN_PIN));
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      digitalWrite(PED_GREEN_PIN, LOW);
      digitalWrite(PED_RED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(3000));

      // 5. Autos de Rojo a Verde y enfriamiento
      systemState = COOLDOWN;
      digitalWrite(CAR_RED_PIN, LOW);
      digitalWrite(CAR_GREEN_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(6000));

      systemState = CAR_GREEN; // Retorno a Verde Autos normal
      
      // Limpiar rebotes acumulados en el botón
      while (xSemaphoreTake(xSemButton, 0) == pdTRUE);
    }
  }
}

// Tarea Periódica de Monitoreo (Prioridad 1 - cada 3 segundos)
void monitorTask(void *pv) {
  (void) pv;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    Serial.print(F("Ticks: "));
    Serial.print(xTaskGetTickCount());
    Serial.print(F(" | State: "));
    
    // Imprimir el nombre del estado correspondiente usando switch-case
    switch (systemState) {
      case CAR_GREEN:     Serial.print(F("CAR_GREEN")); break;
      case CAR_YELLOW:    Serial.print(F("CAR_YELLOW")); break;
      case PED_CROSSING:  Serial.print(F("PED_CROSSING")); break;
      case PED_WARNING:   Serial.print(F("PED_WARNING")); break;
      case COOLDOWN:      Serial.print(F("COOLDOWN")); break;
    }
    
    Serial.print(F(" | Crossings: "));
    Serial.println(requests);
  }
}
