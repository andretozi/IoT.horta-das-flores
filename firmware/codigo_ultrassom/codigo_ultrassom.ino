// Horta das Flores - Grupo 07
// Sensor ultrassônico HC-SR04: mede a distância em cm e publica via MQTT (TLS).
// Volume, nível e percentual são calculados no app.
//
// Biblioteca: PubSubClient (Nick O'Leary), pelo Gerenciador de Bibliotecas.
// Ligações: VCC->VIN (5 V), GND->GND, TRIG->GPIO 26,
//           ECHO->1 kΩ->GPIO 25, e 2 kΩ do GPIO 25 para o GND.
//
// Credenciais ficam em secrets.h (copie de secrets.h.example).

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

const char* ca_cert =
"-----BEGIN CERTIFICATE-----\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
"MrY=\n"
"-----END CERTIFICATE-----\n";

const int PINO_TRIG = 26;
const int PINO_ECHO = 25;
const unsigned long INTERVALO_MS = 10000;

WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;

// Média de 5 leituras. Devolve -1 se o sensor não responder.
float lerDistanciaCm() {
  float soma = 0;
  int validas = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(PINO_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PINO_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PINO_TRIG, LOW);
    unsigned long duracao = pulseIn(PINO_ECHO, HIGH, 30000);
    if (duracao > 0) {
      soma += duracao * 0.0343 / 2.0;  // 0,0343 cm/µs, dividido por 2 (ida e volta)
      validas++;
    }
    delay(60);
  }
  if (validas == 0) return -1;
  return soma / validas;
}

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado");
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker... ");
    if (client.connect(MQTT_CLIENT_ID, MQTT_USUARIO, MQTT_SENHA)) {
      Serial.println("conectado");
    } else {
      // rc 4: usuário/senha errados, 5: sem permissão, -2: rede ou certificado
      Serial.print("falhou, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);

  conectarWiFi();
  espClient.setCACert(ca_cert);
  client.setServer(MQTT_SERVIDOR, MQTT_PORTA);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) conectarWiFi();
  if (!client.connected()) conectarMQTT();
  client.loop();

  if (millis() - ultimoEnvio < INTERVALO_MS) return;
  ultimoEnvio = millis();

  float distancia = lerDistanciaCm();
  if (distancia < 0) {
    Serial.println("Sensor sem leitura. Confira as ligações.");
    return;
  }

  char payload[48];
  snprintf(payload, sizeof(payload), "{\"distancia_cm\":%.1f}", distancia);

  Serial.println(payload);
  Serial.println(client.publish(MQTT_TOPICO, payload) ? "Enviado" : "Falha ao enviar");
}
