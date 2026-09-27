#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Fota.h"
#include "Network.h"

static const char *URL_VERSION =
  "https://raw.githubusercontent.com/Radinux/SilkyOS/master/version.json";

static bool dejaVerifie = false;   // Pour ne vérifier qu'une seule fois (pour l'instant)

static void verifier() {
  // Connexion sécurisée (HTTPS), comme pour la météo
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  http.begin(client, URL_VERSION);
  int code = http.GET();

  // Le serveur n'a pas répondu "OK" : on affiche l'erreur et on arrête
  if (code != HTTP_CODE_OK) {
    Serial.printf("[FOTA] Erreur HTTP %d\n", code);
    http.end();
    return;
  }

  String corps = http.getString();   // Le texte du fichier version.json
  http.end();

  JsonDocument doc;
  deserializeJson(doc, corps);       // On décode le JSON

  // Champ absent ou mal écrit → 0, donc jamais de fausse alerte
  int buildDistant = doc["build"].as<int>();

  Serial.printf("[FOTA] Local %d, distant %d\n", SILKY_BUILD, buildDistant);

  if (buildDistant > SILKY_BUILD) {
    Serial.println("[FOTA] Mise a jour disponible !");
  } else {
    Serial.println("[FOTA] SilkyOS est a jour");
  }
}

void fotaTache() {
  // On attend d'être connecté, et on ne vérifie qu'une fois
  if (!netIsConnected() || dejaVerifie) return;
  dejaVerifie = true;
  verifier();
}