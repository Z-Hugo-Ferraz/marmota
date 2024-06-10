#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Dados.h"

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

static const char htmlAntes[] PROGMEM = "<!DOCTYPE html><html lang=\"pt-br\"><head><meta charset=\"UTF-8\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" /><title>Marmota</title><style type=\"text/css\">body {text-align: center;margin-top: 250px;background: url(https://z-hugo-ferraz.github.io/marmota/Marmota/FundoHD.png);}a {text-decoration: none;color: #ffffff;display: inline-block;background: #8d34e0;padding: 10px 20px;border-radius: 5px;font-size: 60px;}a:hover {background: #7700aa;}a:active {background: #0000ff;}.p1 {text-decoration: none;color: #ffffff;display: inline-block;background: #dd00ff;padding: 10px 20px;border-radius: 5px;font-size: 30px}</style></head><body><p><a href=\"/iniciar\">Iniciar!</a></p>";
static const char htmlDepois[] PROGMEM = "</body></html>\r\n";

void enviarPaginaInicial() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(htmlAntes);
  server.sendContent(htmlDepois);
}

int respTemp;
int acertos;
int pontos;
int streak;
int erros;
int maximo;
int vezAtual;
int tempoAceso;
int pontosPorAcerto;

int led[3] = { D6, D7, D8 };
int botao[3] = { D0, D4, D5 };
// Sorteia Marmota
int sortMarmota() {
  return random(3); // entre 0 e 2 (3 exclui)
}

// Verifica acerto
bool verificaAcerto(int marmotaPiscada){
  long tempoFinal = millis() + respTemp;
  while (millis() < tempoFinal) {
    delay(50);
    for (int i = 0; i < 3; i++) {
      if (digitalRead(botao[i]) == 1) {
        if (i == marmotaPiscada) {
          return true;
        } else {
          return false;
        }
      }
    }
  }
  return false;
}

void loopJogo() {
  // Rodada
  int marmotaPiscada = sortMarmota();
  
  // Apaga tudo
  digitalWrite(led[0], 0);
  digitalWrite(led[1], 0);
  digitalWrite(led[2], 0);
  
  // Jogo
  digitalWrite(led[marmotaPiscada], 1);
  delay(tempoAceso);
  digitalWrite(led[marmotaPiscada], 0);
  
  // Acertou ou nem
  if (verificaAcerto(marmotaPiscada)){
    streak++;
    acertos++;
    pontos = pontos + pontosPorAcerto;
    if (streak >= 5) {
      pontosPorAcerto = pontosPorAcerto + 1;
      streak = 0;
    }
    for (int piscada = 0; piscada < 3; piscada++) {
      digitalWrite(led[0], 1);
      digitalWrite(led[1], 1);
      digitalWrite(led[2], 1);
      delay(100);
      digitalWrite(led[0], 0);
      digitalWrite(led[1], 0);
      digitalWrite(led[2], 0);
      delay(100);
    }
  } else {
    erros++;
    streak = 0;
    pontosPorAcerto = 1;
  }

  vezAtual = vezAtual + 1;
  tempoAceso = tempoAceso - 10;
  respTemp = respTemp - 100;
  Serial.print("streak ");
  Serial.println(streak);
  Serial.print("pontos ");
  Serial.println(pontos);
  Serial.print("erros ");
  Serial.println(erros);
  Serial.print("pontosPorAcerto ");
  Serial.println(pontosPorAcerto);
  Serial.print("vezAtual ");
  Serial.println(vezAtual);
  Serial.print("tempoAceso ");
  Serial.println(tempoAceso);
  Serial.print("respTemp ");
  Serial.println(respTemp);
  Serial.println();
  Serial.println();
  delay(500);
  for (int i = 0; i < 3; i++) {
    if (digitalRead(botao[i]) == 1) {
      i = -1;
    }
    delay(10);
  }
}

void executarJogo() {
  respTemp = 3000;
  acertos = 0;
  pontos = 0;
  streak = 0;
  erros = 0;
  maximo = 10;
  vezAtual = 0;
  tempoAceso = 500;
  pontosPorAcerto = 1;
  
  while (vezAtual < maximo) {
    loopJogo();
    delay(10);
  }
}

void enviarJogo() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  executarJogo();

  server.sendContent(htmlAntes);
  String resultado = "<p class=\"p1\"><b>Pontos:</b> ";
  resultado += pontos;
  resultado += " <b>Erros:</b> ";
  resultado += erros;
  resultado += "</p>";
  server.sendContent(resultado);

  server.sendContent(htmlDepois);
}

void setup() {
  //LED
  pinMode(led[0], OUTPUT);
  pinMode(led[1], OUTPUT);
  pinMode(led[2], OUTPUT);
  
  //BT
  pinMode(botao[0], INPUT);
  pinMode(botao[1], INPUT);
  pinMode(botao[2], INPUT);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

  server.on("/", enviarPaginaInicial);
  server.on("/iniciar", enviarJogo);
  server.onNotFound(enviarPaginaInicial);

  randomSeed(analogRead(0) + millis());

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  MDNS.update();
}
