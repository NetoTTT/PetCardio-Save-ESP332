#include <ArduinoJson.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <IOXhop_FirebaseESP32.h>
#include <WiFiManager.h>  // Biblioteca WiFiManager

// Configurações Firebase
#define FIREBASE_HOST "https://petcardio-9cabf-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "dzAPDgpYKuTFXB368s34aOaAdQjLEOtZ4hBjmzPY"

const int ecgPin = 34;  // Pino do sensor de ECG
int ecgValue = 0;       // Variável para armazenar o valor lido do ECG

// Função de configuração do Wi-Fi usando WiFiManager
#include <WiFi.h>
#include <WiFiManager.h>

void setupWiFi() {
  WiFiManager wifiManager;
  wifiManager.setTimeout(10);  // Tempo limite de 10 segundos para conexão
  
  // Inicia o modo AP se a conexão não for bem-sucedida
  if (!wifiManager.autoConnect("PetCardio", "12345678")) {
    Serial.println("Falha ao conectar, iniciando modo AP manual.");

    // Configura o modo AP manualmente para manter ativo enquanto aguarda conexão
    WiFi.softAP("PetCardio", "12345678");

    // Mantém o AP aberto até que haja uma conexão ou o tempo máximo de espera seja atingido
    unsigned long startAttemptTime = millis();
    while (WiFi.softAPgetStationNum() == 0 && millis() - startAttemptTime < 30000) {
      Serial.println("Aguardando conexão no AP...");
      delay(1000);
    }

    // Reinicia o ESP caso não haja nenhuma conexão após o tempo limite
    if (WiFi.softAPgetStationNum() == 0) {
      Serial.println("Nenhuma conexão no AP, reiniciando...");
      ESP.restart();
    }
  }

  Serial.println("Conectado ao WiFi!");
}


// Tarefa para ler o valor do ECG
void taskReadECG(void* pvParameters) {
  while (true) {
    ecgValue = analogRead(ecgPin);  // Lê o valor do ECG
    Serial.print("ECG Data: ");
    Serial.println(ecgValue);
    vTaskDelay(50 / portTICK_PERIOD_MS);  // Lê a cada 50ms para aumentar a precisão da leitura
  }
}

// Tarefa para enviar dados para o Firebase
void taskSendECG(void* pvParameters) {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      // Cria o JSON para o dado de ECG
      DynamicJsonBuffer jsonBuffer;
      JsonObject& jsonDoc = jsonBuffer.createObject();
      jsonDoc["ecgValue"] = ecgValue;
      jsonDoc["timestamp"] = millis();

      // Serializa o JSON para uma string
      String jsonData;
      jsonDoc.printTo(jsonData);

      // Envia o JSON para o Firebase
      if (Firebase.push("/ecgData", jsonData)) {
        Serial.println("Dados de ECG enviados com sucesso");
      } else {
        Serial.println("Erro ao enviar dados para o Firebase");
      }
    } else {
      Serial.println("WiFi desconectado. Tentando reconectar...");
      setupWiFi();
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);  // Envia a cada 200ms
  }
}


void setup() {
  Serial.begin(115200);
  setupWiFi();  // Conectar ao Wi-Fi ou configurar AP

  // Inicializar o Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // Criar as tarefas FreeRTOS
  xTaskCreate(taskReadECG, "Ler ECG", 1000, NULL, 1, NULL);
  xTaskCreate(taskSendECG, "Enviar ECG", 4000, NULL, 1, NULL);
}

void loop() {
  // O FreeRTOS gerencia as tarefas, então o loop principal pode permanecer vazio
}