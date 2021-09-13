/*
    Projeto: Análise de Consumo Elétrico com ESP8266
    Autor: Daniel Fiuza - AutoCore Robótica
    Data: 03/04/2020
    Código de Dominio Público.
*/
//============== Declaração de Bibliotecas ====================
#include <NTPClient.h>    // Biblioteca do NTP.
#include <WiFiUdp.h>      // Biblioteca do UDP.
#include <ESP8266WiFi.h>  // Biblioteca do WiFi.
#include <Ultrasonic.h>   // Biblioteca do sensor de distância.
#include <PubSubClient.h> // Biblioteca para enviar dados MQTT.
#include <ArduinoJson.h>  // Biblioteca para criar mensagem Json.

//============== Definição de Constantes ======================
#define WIFI_SSID "WIFI"
#define WIFI_PASS  "PASS"

//Define os pinos para o trigger e echo
#define pino_trigger 2
#define pino_echo 0
#define rele_pin 13


//============== Criação de Instâncias ========================
WiFiUDP udp; // Cria um objeto "UDP".
NTPClient ntp(udp, "0.br.pool.ntp.org", -3 * 3600, 60000); // Cria um objeto "NTP".
WiFiClient wifiClient; // Cria o objeto wifiClient
PubSubClient client(wifiClient); // Cria objeto MQTT client.
Ultrasonic ultrasonic(pino_trigger, pino_echo);

//============= Declaração de Variáveis Globais ===============
int hour; // Armazena o horário obtido do NTP.
int last_hour; // Armazena horário da última medida realizada.
int diff_hour; // Armazena diferença em segundos entre duas medidas.
unsigned long last_time; // Armazena intervalo de tempo entre medidas.
int status = WL_IDLE_STATUS; // Obtém status de conexão Wifi.
int measure = 0;      // variavel para ler dados recebidos pela serial
double economia = 0;
int rele, activator = 0;

//============ Configuração da função void setup() ============
void setup()
{
   Serial.begin(115200);//Inicia a comunicação serial.

   client.setServer("mqtt.eclipseprojects.io", 1883); // Configura conexão MQTT.
   WiFi.begin(WIFI_SSID, WIFI_PASS);
   while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
   }
   ntp.begin(); // Inicia o cliente NTP.
   ntp.forceUpdate();  // Força o Update.
   last_hour = ntp.getEpochTime(); // Armazena primeira medida com a hora atual.
   Serial.println("Lendo dados do sensor...");
   pinMode(rele_pin, OUTPUT); // seta o pino como saída
   digitalWrite(rele_pin, LOW); // seta o pino com nivel logico baixo
}

//============ Configuração da função void loop() ============
void loop(){
   if ( !client.connected() ) {
    reconnect();  // Conecta-se ao broker MQTT.
   }
   
   client.loop(); // Mantém conexão MQTT aberta.
   // Atualiza novas medidas a cada 1s.

   if ( millis() - last_time > 1000 ) {

    //Le as informacoes do sensor, em cm e pol
    float cmMsec=0;
    long microsec = ultrasonic.timing();
    cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);

    Serial.print("Distancia em cm: ");
    Serial.println(cmMsec);
    if(cmMsec <= 65.00){
        digitalWrite(rele_pin,HIGH);  
        activator = 0;
    }

    else{  
      activator++;
      if (activator >= 5){
        digitalWrite(rele_pin,LOW);
        rele += 1;
        economia += 0.04;
      }
    }
    Serial.print(economia); 
    ntp.update();
    hour = (int) ntp.getEpochTime(); // Obtém horário em épocas.

    // Atualiza diferença entre horários em segundos.
    if(last_hour != hour){
      diff_hour = hour - last_hour;
      last_hour = hour;
    }
    
    
    // Envia dados do sensor ao Broker MQTT no formato Json
    StaticJsonDocument<200> datas;
    datas["Economia"] = economia;
    datas["Status"] = rele;
    datas["Hour"] = hour;
    datas["Diff_hour"] = diff_hour;
    String payload;
    serializeJson(datas, payload);
    client.publish("energymeterpi3teste/send", payload.c_str()); // Publica mensagem MQTT
    last_time = millis();
   } 
}

//============ Configuração da função void reconnect() ============
void reconnect() {
  while (!client.connected()) {
    status = WiFi.status();
    // Caso não esteja conectado ao wifi, inicia conexão
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
 
      Serial.println("Conectado ao AP");
    }
    Serial.print("Conectando-se ao Broker MQTT ...");
    // Inicia conexão MQTT
    if ( client.connect("ESP8266 Device") ) {
      Serial.println( "[MQTT DONE]" ); // Conexão MQTT estabelecida.
    } else {
      Serial.print( "[MQTT FAILED] [ rc = " ); // Conexão MQTT falhou.
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Aguarda 5 segundos antes de se reconectar
      delay( 5000 );
    }
  }
}
