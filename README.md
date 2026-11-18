# Horta das Flores - IoT

Projeto da disciplina de Desenvolvimento de Sistemas IoT, Universidade Presbiteriana Mackenzie.

**Integrantes:** Guilherme Sanches e André Magalhães

## O que o projeto faz

Um ESP32 com sensor ultrassônico HC-SR04 mede a distância até a superfície da água do reservatório e envia o valor em centímetros, por MQTT com TLS, para um broker EMQX. Volume, nível e percentual são calculados depois, no aplicativo. O ESP32 só mede e publica.

```
HC-SR04 --> ESP32 --Wi-Fi--> Broker MQTT (EMQX, TLS 8883) --> aplicativo
```

## Estrutura

```
firmware/codigo_ultrassom/
  codigo_ultrassom.ino     código do ESP32
  secrets.h.example     modelo das credenciais (copie para secrets.h)
```

O `secrets.h` guarda Wi-Fi e senha do broker e **não vai para o GitHub** (está no `.gitignore`).

## Como usar

1. Instale a IDE do Arduino e o pacote de placas ESP32. Escolha **ESP32 Dev Module**.
2. Instale a biblioteca **PubSubClient** (Nick O'Leary) pelo Gerenciador de Bibliotecas.
3. Em `firmware/codigo_ultrassom/`, copie `secrets.h.example` para `secrets.h` e preencha Wi-Fi (rede 2,4 GHz) e dados do broker.
4. Abra `codigo_ultrassom.ino`, compile e envie para a placa.
5. Abra o Serial Monitor em 115200 para acompanhar.

## Ligações

| HC-SR04 | ESP32 |
|---|---|
| VCC | VIN (5 V) |
| GND | GND |
| TRIG | GPIO 26 |
| ECHO | 1 kΩ em série até o GPIO 25, com 2 kΩ do GPIO 25 para o GND |

O ECHO sai em 5 V e o ESP32 não aceita essa tensão no pino. O divisor de tensão reduz para cerca de 3,3 V (5 × 2 / 3).

## Explicação do código

### Bibliotecas

| Biblioteca | Função |
|---|---|
| `WiFi.h` | Conecta o ESP32 à rede Wi-Fi. |
| `WiFiClientSecure.h` | Cria a conexão segura (TLS) com o broker. |
| `PubSubClient.h` | Implementa o MQTT: conectar, publicar e manter a conexão. |

As três trabalham em camadas: Wi-Fi dá a rede, `WiFiClientSecure` criptografa por cima, e `PubSubClient` fala MQTT por esse canal.

### Configuração

- `ca_cert`: certificado raiz (DigiCert Global Root G2) usado para conferir a identidade do broker.
- `PINO_TRIG` (26) e `PINO_ECHO` (25): pinos do sensor.
- `INTERVALO_MS`: intervalo entre envios, 10 segundos.
- As credenciais (Wi-Fi, servidor, usuário, senha e tópico) vêm do `secrets.h`.

### `lerDistanciaCm()`

Mede a distância e devolve o resultado em centímetros:

1. Coloca o TRIG em LOW por 2 µs, em HIGH por 10 µs e volta a LOW. Esse pulso dispara o sensor.
2. `pulseIn(PINO_ECHO, HIGH, 30000)` mede, em microssegundos, quanto tempo o ECHO fica em HIGH. É o tempo de ida e volta do som. O limite de 30 ms evita travar se não houver eco.
3. Converte para centímetros: `distância = duração × 0,0343 ÷ 2`. O valor 0,0343 é a velocidade do som em cm/µs, e a divisão por 2 existe porque o som vai e volta.
4. Repete 5 vezes, com 60 ms entre as medições, e faz a média das leituras válidas.
5. Se nenhuma leitura for válida, devolve `-1`.

### `conectarWiFi()` e `conectarMQTT()`

- `conectarWiFi()` chama `WiFi.begin` e espera `WL_CONNECTED`.
- `conectarMQTT()` tenta `client.connect` até conseguir. Em caso de falha, imprime o código `rc` e tenta de novo em 5 s. Códigos comuns: 4 (usuário ou senha errados), 5 (sem permissão), -2 (rede ou certificado).

### `setup()`

Inicia a serial, configura os pinos, conecta ao Wi-Fi, instala o certificado com `setCACert` e define o broker com `setServer`.

### `loop()`

1. Reconecta ao Wi-Fi e ao broker se a conexão cair.
2. `client.loop()` mantém a conexão MQTT viva.
3. Usa `millis()` para enviar a cada 10 s sem bloquear o programa (com `delay`, o MQTT pararia de ser atendido).
4. Lê a distância. Se vier `-1`, avisa no monitor serial e não publica.
5. Monta o JSON e publica no tópico.

## Mensagem publicada

Tópico `horta-das-flores/sensor-ultrason/telemetria`, a cada 10 segundos:

```json
{"distancia_cm":12.3}
```
