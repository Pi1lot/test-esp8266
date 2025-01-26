#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Configuration Wi-Fi
const char* ssid = "Your SSID";
const char* password = "Your password";

// Configuration API Steam
const char* STEAM_API_KEY = "Your steam API key";
const char* STEAM_ID = "Your steamID";
String apiUrl = "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/";

// URL de base pour les images, elle sera générée dynamiquement selon l'ID du jeu
String imageUrlBase = "https://raw.githubusercontent.com/Pi1lot/test-esp8266/main/";

// Variables pour afficher le jeu et le temps
String currentGame = "Aucun jeu";
unsigned long gameStartTime = 0; // Temps de début du jeu en secondes

// Variables pour télécharger l'image
#define IMAGE_WIDTH  48 // Largeur de l'image (multiple de 8)
#define IMAGE_HEIGHT 48 // Hauteur de l'image (multiple de 8)
#define EXPECTED_SIZE (IMAGE_HEIGHT * IMAGE_WIDTH / 8) // Taille attendue de l'image
uint8_t imageBuffer[EXPECTED_SIZE] = {0}; // Buffer pour stocker l'image téléchargée

// Délai pour les requêtes API
unsigned long previousMillis = 0;
const unsigned long interval = 5000;

// Initialiser l'écran U8g2
U8G2_GP1287AI_256X50_F_4W_HW_SPI u8g2(U8G2_R2, 15, U8X8_PIN_NONE, 16);

// Créer une instance de WiFiClientSecure pour la gestion HTTPS
WiFiClientSecure client;

// Configurer le NTP
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000);  // Décalage à 0 pour UTC, actualisation toutes les 60 secondes

void setup() {
  Serial.begin(115200);

  // Initialiser l'écran
  u8g2.begin();
  u8g2.setContrast(255); // Luminosité maximale
  u8g2.clearDisplay();

  // Connexion au Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\nWi-Fi connecté !");
  
  // Initialiser le client NTP
  timeClient.begin();
  timeClient.setTimeOffset(3600);  // Ajuster pour le fuseau horaire (ex: 3600 pour UTC+1)
}

void loop() {
  // Effectuer un appel API toutes les 5 secondes
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    updateCurrentGame(); // Récupérer les informations du jeu actuel
  }

  // Mettre à jour l'heure NTP
  timeClient.update();

  // Afficher les informations sur l'écran
  displayGameInfo();
}

void updateCurrentGame() {
  if (WiFi.status() == WL_CONNECTED) {
    // Utiliser BearSSL pour HTTPS
    client.setInsecure(); // Désactiver la vérification SSL

    HTTPClient https;
    String url = apiUrl + "?key=" + STEAM_API_KEY + "&steamids=" + STEAM_ID;

    if (https.begin(client, url)) {
      int httpCode = https.GET();

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = https.getString();

          
          StaticJsonDocument<1096> doc; //Il faut bien augmenter la taille SINON certains jeu avec un long nom ne marchent pas!

          
          DeserializationError error = deserializeJson(doc, payload);

          if (!error) {
            JsonArray players = doc["response"]["players"].as<JsonArray>();
            if (players.size() > 0) {
              const char* gameName = players[0]["gameextrainfo"];
              const char* gameId = players[0]["gameid"];

              if (gameName) {
                if (currentGame != String(gameName)) {
                  currentGame = String(gameName);
                  gameStartTime = millis() / 1000; // Définir le début du jeu
                  // Générer l'URL de l'image dynamique
                  String imageUrl = imageUrlBase + String(gameId) + ".raw";
                  // Télécharger l'image si un jeu est détecté
                  if (downloadImage(imageUrl)) {
                    Serial.println("Image téléchargée et affichée !");
                  }
                }
              } else {
                currentGame = "Aucun jeu";
                gameStartTime = 0;
                // Télécharger l'image 0.raw lorsqu'aucun jeu n'est lancé
                downloadImage(imageUrlBase + "0.raw");
              }
            }
          }
        }
      } else {
        Serial.printf("Erreur lors de l'appel à l'API : %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.println("Impossible de se connecter à l'API Steam !");
    }
  } else {
    Serial.println("Wi-Fi non connecté !");
  }
}

bool downloadImage(String imageUrl) {
  HTTPClient https;
  if (https.begin(client, imageUrl)) {
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
      WiFiClient* stream = https.getStreamPtr();
      size_t index = 0;

      while (stream->connected() && stream->available() && index < sizeof(imageBuffer)) {
        imageBuffer[index++] = stream->read();
      }

      if (index == EXPECTED_SIZE) {
        Serial.println("[HTTPS] Image téléchargée avec succès !");
        https.end();
        invertImage(); // Inverser l'image
        return true;
      } else {
        Serial.println("[HTTPS] Taille du fichier incorrecte !");
      }
    } else {
      Serial.printf("[HTTPS] Erreur : %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.println("[HTTPS] Impossible de se connecter à l'URL de l'image !");
  }
  return false;
}

void invertImage() {
  for (int i = 0; i < IMAGE_HEIGHT; i++) {
    for (int j = 0; j < IMAGE_WIDTH / 8; j++) {
      uint8_t byte = imageBuffer[i * (IMAGE_WIDTH / 8) + j];
      imageBuffer[i * (IMAGE_WIDTH / 8) + j] = reverseBits(byte);
    }
  }
}

uint8_t reverseBits(uint8_t byte) {
  uint8_t reversed = 0;
  for (int i = 0; i < 8; i++) {
    reversed = (reversed << 1) | (byte & 1);
    byte >>= 1;
  }
  return reversed;
}

void displayGameInfo() {
  // Calculer l'heure NTP
  String formattedTime = timeClient.getFormattedTime();
  // Calculer le temps écoulé depuis le début du jeu
  unsigned long elapsedTime = 0;
  if (gameStartTime > 0) {
    elapsedTime = (millis() / 1000) - gameStartTime;
  }

  // Préparer le texte à afficher
  char timeString[22];
  snprintf(timeString, sizeof(timeString), "Session: %02lu:%02lu:%02lu",
           elapsedTime / 3600, (elapsedTime / 60) % 60, elapsedTime % 60); // Heures:Minutes:Secondes
  // Afficher sur l'écran
  u8g2.clearBuffer();  // Effacer uniquement la partie du texte

  if (currentGame == "Aucun jeu") {
    // Afficher "IDLE" avec une police plus grande si aucun jeu n'est lancé
    u8g2.setFont(u8g2_font_spleen12x24_mr); // Police plus grande pour "IDLE"
    u8g2.drawStr(0, 20, "IDLE");
        // Afficher une barre sous le nom du jeu
    u8g2.drawHLine(0, 24, 128);
    u8g2.drawHLine(0, 25, 128);
    u8g2.drawHLine(0, 26, 127);
    u8g2.drawHLine(0, 27, 126);
    // Afficher l'heure via NTP
    u8g2.setFont(u8g2_font_spleen6x12_mr); // Police plus petite pour l'heure
    u8g2.drawStr(0, 43, formattedTime.c_str()); // Afficher l'heure
  } else {


    
    // Si un jeu est en cours, ajuster la police selon la longueur du nom du jeu
    if (currentGame.length() > 18) {
      u8g2.setFont(u8g2_font_spleen6x12_mr); // Police plus petite si le nom du jeu est long
      u8g2.drawStr(0, 16, currentGame.c_str());
    } else {
      u8g2.setFont(u8g2_font_spleen12x24_mr); // Police plus grande pour un nom court
          // Afficher le nom du jeu
      u8g2.drawStr(0, 20, currentGame.c_str());
    }


    // Afficher une barre sous le nom du jeu
    u8g2.drawHLine(0, 24, 128);
    u8g2.drawHLine(0, 25, 128);
    u8g2.drawHLine(0, 26, 127);
    u8g2.drawHLine(0, 27, 126);

    // Afficher le temps écoulé
    u8g2.setFont(u8g2_font_spleen6x12_mr); // Police plus petite pour le temps
    u8g2.drawStr(0, 43, timeString); // Temps écoulé
  }

  // Afficher l'image à droite (en supposant qu'elle est téléchargée correctement)
  u8g2.drawXBMP(207, 1, IMAGE_WIDTH, IMAGE_HEIGHT, imageBuffer);

  // Envoyer les données à l'écran
  u8g2.sendBuffer();
}
