/*
   Copyright 2016 Benjamin Allegre

   This file is part of lan-nfc-computer-unlocker.
   https://github.com/sigfrodr/lan-nfc-computer-unlocker

   lan-nfc-computer-unlocker is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   lan-nfc-computer-unlocker is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with lan-nfc-computer-unlocker.  If not, see <http://www.gnu.org/licenses/>.

   Hardware requirements:
   - Arduino Leonardo
   - Adafruit PN532 shield
   - Ethernet shield

   The following libraries are used:
   - https://github.com/adafruit/Adafruit-PN532
   - https://github.com/bblanchon/ArduinoJson
   - https://github.com/agdl/Base64
*/

#include <Keyboard.h>
#include <Adafruit_PN532.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <Base64.h>

/* ----- Configure Adafruit PN532 ----- */

#define IRQ 6
#define RESET 8

Adafruit_PN532 nfc(IRQ, RESET);

/* ----- Configure Ethernet ----- */

const char server[] = "192.168.1.11"; // Lan server
String subfolder = "/lancer/public";
EthernetClient client;

/* ----- Configure Encryption ----- */

const char key[] = "lancer";    // Encryption key (must match server key)
const uint32_t keyLen = strlen(key);

/* ----- Configure OS ----- */

bool W10 = true;

/* ----- SETUP ----- */

void setup()
{
  initSerial();
  initNFC();
  initEthernet();
  Keyboard.begin();
}

/* ----- LOOP ----- */

void loop()
{
  uint32_t cardidentifier = waitForRFIDCard();

  // Check the webservice
  if (connect())
  {
    //if (sendRequest(cardidentifier) && skipResponseHeaders())
    String request = String("GET ") + subfolder + String("/user/") + cardidentifier + String(" HTTP/1.1");
    sendRequest(request);

    if (skipResponseHeaders())
    {
      char response[512];
      readReponseContent(response, sizeof(response));

      char encPassword[100];
      if ( parseUserData(response, encPassword) )
      {
        int encPasswordLen = strlen(encPassword);
        int decodedLen = Base64.decodedLength(encPassword, encPasswordLen);

        //Serial.print("decodeLen: "); Serial.println(decodedLen);

        char decoded[decodedLen];
        Base64.decode(decoded, encPassword, encPasswordLen);

        //Serial.print(encPassword); Serial.print(" #"); Serial.print(encPasswordLen); Serial.print(" = "); Serial.println(decoded);

        char password[100];
        decrypt(decoded, password);

        //Serial.println("Password: "); Serial.println(password);

        login(password);
      }
    }

    disconnect();
  }
}

/* ----- FUNCTIONS ----- */

/* Initialize serial port */

void initSerial() {
  Serial.begin(9600);
  while (!Serial) {
    ;  // Wait for serial port to initialize
  }
  Serial.println("Serial ready");
}


/* Initialize NFC */

void initNFC() {
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // Stop
  }

  // Got ok data, print it out!
  /* Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC); */

  // Configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("NFC ready");
}


/* Initialize Ethernet */

void initEthernet() {
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  if (!Ethernet.begin(mac)) {
    Serial.println("Failed to configure Ethernet");
    return;
  }
  Serial.println("Ethernet ready");
  delay(1000);

  // Print local IP address:
  /* Serial.print("IP: ");
    for (byte thisByte = 0; thisByte < 4; thisByte++) { */
  // Print the value of each byte of the IP address:
  /* Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
    }
    Serial.println(); */
}


/* Wait for an RFID card and return card identifier when found */

uint32_t waitForRFIDCard()
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  Serial.println("Waiting for an ISO14443A Card ...");

  // Wait for an ISO14443A type cards (Mifare, etc.). When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  // success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  uint32_t cardidentifier = 0;

  if (success)
  {
    Serial.print("Card detected: ");
    // Turn the four byte UID of a mifare classic into a single variable #
    cardidentifier = uid[3];
    cardidentifier <<= 8; cardidentifier |= uid[2];
    cardidentifier <<= 8; cardidentifier |= uid[1];
    cardidentifier <<= 8; cardidentifier |= uid[0];
    Serial.println(cardidentifier);
  }

  return cardidentifier;
}


/* Skip HTTP headers so that we are at the beginning of the response's body */

bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }

  return ok;
}


/* Open connection to the HTTP server */

bool connect()
{
  Serial.println(String("Connecting to ") + server + String(" ..."));

  bool ok = client.connect(server, 80);

  Serial.println(ok ? "Connected!" : "Connection Failed!");
  return ok;
}

/* Send the HTTP GET request to the server */

void sendRequest(String request)
{
  Serial.println(request);

  client.println(request);
  client.println(String("Host: ") + server);
  client.println("Connection: close");
  client.println();
}


/* Read the body of the response from the HTTP server */

void readReponseContent(char* content, size_t maxSize) {
  size_t length = client.readBytes(content, maxSize);
  content[length] = 0;
  Serial.println(content);
}


/* Parse user data from JSON */

bool parseUserData(char* content, char *encPassword) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  const size_t BUFFER_SIZE =
    JSON_OBJECT_SIZE(4);     // the root object has 4 element

  // Allocate a temporary memory pool on the stack
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  // If the memory pool is too big for the stack, use this instead:
  // DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  if (root.containsKey("error"))
  {
    Serial.println("Unrecognized card. Access denied.");
    return false;
  }

  // Extract password
  strcpy(encPassword, root["password"]);

  return true;
}


/* Close the connection with the HTTP server */

void disconnect() {
  client.stop();
  Serial.println("Disconnected");
}


/* Decrypt the password */

void decrypt(char* decoded, char* password)
{
  uint32_t decodedLen = strlen(decoded);
  char decrypted[decodedLen + 1];

  for (int i = 0; i < decodedLen; i++)
    decrypted[i] = decoded[i] ^ key[i % keyLen];

  decrypted[decodedLen] = '\0';

  strcpy(password, decrypted);
}


/* Log user to the computer */

void login(char* password) {
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(KEY_DELETE);
  delay(150);
  Keyboard.releaseAll();
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press('v');
  delay(600);
  Keyboard.releaseAll();

  // Windows 10
  if (W10)
  {
    delay(600);
    Keyboard.press(KEY_BACKSPACE);
    delay(600);
  }

  Keyboard.print(password);
  Keyboard.write(KEY_RETURN);
  delay(5000); // Make sure the password isn't repeated
}
