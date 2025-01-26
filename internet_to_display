#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <U8g2lib.h>

// Définir les identifiants WiFi
const char* ssid = "SSID";
const char* password = "PASSWORD";

// URL de l'image RAW
const char* image_url = "https://raw.githubusercontent.com/Pi1lot/test-esp8266/main/debug_48x48.raw"; // Doit être au format raw et les dimensions doivent êtres des multiples de 8
// Définir la taille de l'image
#define IMAGE_WIDTH  48 //Doit être un multiple de 8
#define IMAGE_HEIGHT 48 //Doit être un multiple de 8
#define EXPECTED_SIZE (IMAGE_HEIGHT * IMAGE_WIDTH / 8) //Taille du fichier raw, si les dimensions sont respectées, la formule est cohérente
uint8_t imageBuffer[EXPECTED_SIZE] = {0}; // Buffer pour stocker les données RAW

// Initialiser l'écran U8g2
U8G2_GP1287AI_256X50_F_4W_HW_SPI u8g2(/*Rotation de 180°*/U8G2_R2, /* cs=*/15, /* dc=*/U8X8_PIN_NONE, /* reset=*/16); // display initiliazation

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Initialiser l'écran
  u8g2.begin();
  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 12, "Connexion WiFi...");
  u8g2.sendBuffer();

  // Connexion au WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\nConnecté au WiFi !");
  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "Connecté !");
  u8g2.sendBuffer();

  // Téléchargement de l'image
  if (downloadImage()) {
    Serial.println("Téléchargement terminé !");
    invertImage(); // Inverser l'image avant de l'afficher
    displayImage(); // Afficher l'image téléchargée
  } else {
    Serial.println("Erreur de téléchargement !");
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Erreur telechargement");
    u8g2.sendBuffer();
  }
}

void loop() {
  // Rien dans la boucle principale pour l'instant
}

// Fonction pour télécharger l'image RAW
bool downloadImage() {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure(); // Désactiver la vérification SSL

  HTTPClient https;
  if (https.begin(*client, image_url)) {
    Serial.println("[HTTPS] Téléchargement de l'image RAW...");
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
      WiFiClient* stream = https.getStreamPtr();
      size_t index = 0;

      // Lire les données RAW dans le buffer
      while (stream->connected() && stream->available() && index < sizeof(imageBuffer)) {
        imageBuffer[index++] = stream->read();
      }

      // Afficher la taille des données téléchargées
      Serial.printf("Taille des données téléchargées : %u octets\n", index);

      // Vérifier la taille attendue
      if (index == EXPECTED_SIZE) {
        Serial.println("[HTTPS] Image téléchargée avec succès !");
        https.end();
        return true;
      } else {
        Serial.printf("[HTTPS] Taille du fichier incorrecte : %u octets\n", index);
      }
    } else {
      Serial.printf("[HTTPS] Erreur : %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.println("[HTTPS] Impossible de se connecter à l'URL !");
  }
  return false;
}

// Fonction pour inverser horizontalement l'image
void invertImage() {
  for (int i = 0; i < IMAGE_HEIGHT; i++) {
    // Inverser les octets sur chaque ligne
    for (int j = 0; j < IMAGE_WIDTH / 8; j++) {
      uint8_t byte = imageBuffer[i * (IMAGE_WIDTH / 8) + j];
      imageBuffer[i * (IMAGE_WIDTH / 8) + j] = reverseBits(byte);
    }
  }
}

// Fonction pour inverser les bits d'un octet
uint8_t reverseBits(uint8_t byte) {
  uint8_t reversed = 0;
  for (int i = 0; i < 8; i++) {
    reversed = (reversed << 1) | (byte & 1);
    byte >>= 1;
  }
  return reversed;
}

// Fonction pour afficher l'image sur l'écran
void displayImage() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "Affichage Image...");
  u8g2.sendBuffer();

  delay(1000);

  // Dessiner l'image téléchargée
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, imageBuffer); // Affiche l'image centrée
  u8g2.sendBuffer();

  Serial.println("[INFO] Image affichée !");
}
