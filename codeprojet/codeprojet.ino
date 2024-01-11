#include <Adafruit_Fingerprint.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// Pour UNO et autres sans matériel série, nous devons utiliser le logiciel série...
// La broche #2 est en provenance du capteur (fil VERT)
// La broche #3 est en provenance de l'Arduino  (fil BLANC)
// Configurer le port série pour utiliser le logiciel série...
SoftwareSerial mySerial(2, 3);

#else
// Sur Leonardo/M0/etc., et autres avec matériel série, utilisez le matériel série !
// #0 est le fil vert, #1 est le fil blanc
#define mySerial Serial1

#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

void setup()
{
  Serial.begin(9600);
  while (!Serial);  // Pour Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nGestion du capteur d'empreintes Adafruit");

  // Définir le débit de données pour le port série du capteur
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Capteur d'empreintes trouvé !");
  } else {
    Serial.println("Capteur d'empreintes non trouvé :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Lecture des paramètres du capteur"));
  finger.getParameters();
  Serial.print(F("Statut : 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("ID Système : 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacité : ")); Serial.println(finger.capacity);
  Serial.print(F("Niveau de sécurité : ")); Serial.println(finger.security_level);
  Serial.print(F("Adresse du périphérique : ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Longueur du paquet : ")); Serial.println(finger.packet_len);
  Serial.print(F("Débit de bauds : ")); Serial.println(finger.baud_rate);
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop()                     // exécuté en boucle
{
  Serial.println("Que souhaitez-vous faire ?");
  Serial.println("1 - Enregistrer une nouvelle empreinte");
  Serial.println("2 - Supprimer une empreinte existante");
  Serial.println("3 - Vérifier l'empreinte");

  int choice = readnumber();

  if (choice == 1) {
    enrollFingerprint();
  } else if (choice == 2) {
    deleteFingerprint();
  } else if (choice == 3) {
    verifyFingerprint();
  }
}

void enrollFingerprint() {
  Serial.println("Prêt à enregistrer une empreinte !");
  Serial.println("Veuillez entrer le numéro d'identification (de 1 à 127) que vous souhaitez attribuer à cette empreinte...");
  id = readnumber();
  if (id == 0) {// L'ID #0 n'est pas autorisé, réessayez !
     return;
  }
  Serial.print("Enregistrement de l'ID #");
  Serial.println(id);

  while (!getFingerprintEnroll());
}

void deleteFingerprint() {
  Serial.println("Prêt à supprimer une empreinte !");
  Serial.println("Veuillez entrer le numéro d'identification (ID) de l'empreinte à supprimer...");
  id = readnumber();

  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Empreinte supprimée !");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erreur de communication");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Impossible de trouver une empreinte à cet emplacement");
  } else {
    Serial.println("Erreur inconnue");
  }
}

void verifyFingerprint() {
  Serial.println("Prêt à vérifier l'empreinte !");
  Serial.println("Veuillez placer votre doigt sur le capteur...");

  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Empreinte capturée");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("Aucun doigt détecté.");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Erreur de communication");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Erreur d'imagerie");
        break;
      default:
        Serial.println("Erreur inconnue");
        break;
    }
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("Impossible de convertir l'image. Réessayez.");
    return;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Empreinte reconnue !");
  } else {
    Serial.println("Empreinte non reconnue. Veuillez réessayer.");
  }
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("En attente d'une empreinte valide pour l'enregistrement en tant que #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Empreinte capturée");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Erreur de communication");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Erreur d'imagerie");
        break;
      default:
        Serial.println("Erreur inconnue");
        break;
    }
  }

  // OK, succès !

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image convertie");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image trop floue");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erreur de communication");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
      return p;
    default:
      Serial.println("Erreur inconnue");
      return p;
  }

  Serial.println("Retirez le doigt");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Placez le même doigt à nouveau");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Empreinte capturée");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Erreur de communication");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Erreur d'imagerie");
        break;
      default:
        Serial.println("Erreur inconnue");
        break;
    }
  }

  // OK, succès !

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image convertie");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image trop floue");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erreur de communication");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
      return p;
    default:
      Serial.println("Erreur inconnue");
      return p;
  }

  // OK, converti !
  Serial.print("Création du modèle pour #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Empreintes correspondantes !");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erreur de communication");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Les empreintes ne correspondent pas");
    return p;
  } else {
    Serial.println("Erreur inconnue");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Enregistré !");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erreur de communication");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Impossible de stocker à cet emplacement");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Erreur d'écriture sur la mémoire flash");
    return p;
  } else {
    Serial.println("Erreur inconnue");
    return p;
  }

  return true;
}



