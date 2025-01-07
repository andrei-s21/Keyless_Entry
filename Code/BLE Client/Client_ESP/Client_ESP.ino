#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <mbedtls/aes.h>

uint8_t masterKey[16] = {0x00, 0x00 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //replace with your master key
uint8_t sendData[16] = {0x00};
uint8_t encryptedMessage[16] = {0x00};
uint8_t sessionKey[16];
uint8_t tokenServer[4];
uint8_t tokenClient[4];
uint8_t receivedData[16];
uint8_t counter = 0;
unsigned long tm;
BLEClient* pClient;
BLERemoteCharacteristic* characteristicSend = nullptr;
BLERemoteCharacteristic* characteristicReceive = nullptr;
bool handshakeComplete = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ENGINE_BTN 25
#define LIGHTS_BTN 26
#define SCREEN_BTN 27
#define UNLOCK_BTN 14
#define LOCKRIO_BTN 12

#define SERVICE_UUID "5daea2b4-1912-4839-8c9d-7657f074fa84"
#define TARGET_CHARACTERISTIC_UUID_RECEIVE "df847cb0-69aa-43e1-8aa9-1fad496c60b5"
#define TARGET_CHARACTERISTIC_UUID_SEND "f61ad6ec-247b-44e9-98e1-f2a61b9836ea"

bool connected = false;
bool scanning = false;
BLEScan* pBLEScan;
BLEAddress* pServerAddress = nullptr;

unsigned long lastButtonPress = 0;
const unsigned long OLED_TIMEOUT = 5000;

uint8_t checksum(uint8_t* array) {
    uint8_t count = 0;
    uint8_t byteValue = 0;
    for (short i = 0; i < 15; i++) {
        if (i != 13) {
            byteValue = array[i];
            while (byteValue) {
                count += byteValue & 1;
                byteValue >>= 1;
            }
        }
    }
    return count;
}

void encrypt(uint8_t* input, uint8_t* key, uint8_t* output) {
    uint8_t iv[16] = {0};
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 16, iv, input, output);
}

void decrypt(uint8_t* input, uint8_t* key, uint8_t* output) {
    uint8_t iv[16] = {0};
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 16, iv, input, output);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
            pBLEScan->stop();
            scanning = false;
        }
    }
};

class MyClientCallbacks : public BLEClientCallbacks {
    void onConnect(BLEClient* client) override {
        connected = true;
    }

    void onDisconnect(BLEClient* client) override {
        connected = false;
        delete pServerAddress;
        pServerAddress = nullptr;
        handshakeComplete = false;
        counter = 0;
    }
};

void performHandshake() {
    for (int i = 0; i < 16; i++) sessionKey[i] = random(0, 255);
    encrypt(sessionKey, masterKey, encryptedMessage);
    characteristicSend->writeValue(encryptedMessage, 16);

    delay(400);
    decrypt(characteristicReceive->readRawData(), sessionKey, receivedData);

    if (checksum(receivedData) == receivedData[15]) {
        for (int i = 0; i < 4; i++) {
            tokenServer[i] = receivedData[i + 2];
            tokenClient[i] = random(0, 255);
        }
    }else pClient->disconnect();
    sendData[0] = 0x21;
    sendData[1] = 0xB3;
    sendData[2] = tokenClient[0];
    sendData[3] = tokenClient[1];
    sendData[4] = tokenClient[2];
    sendData[5] = tokenClient[3];
    sendData[6] = tokenServer[0];
    sendData[7] = tokenServer[1];
    sendData[8] = tokenServer[2];
    sendData[9] = tokenServer[3];
    sendData[10] = 0x00;
    sendData[11] = (byte)random(0, 255);
    sendData[12] = 0x00;
    sendData[13] = 0x00;
    sendData[14] = 0x00;
    sendData[15] = checksum(sendData);

    encrypt(sendData, sessionKey, encryptedMessage);
    characteristicSend->writeValue(encryptedMessage, 16);

    delay(400);
    decrypt(characteristicReceive->readRawData(), sessionKey, receivedData);

    if (checksum(receivedData) == receivedData[15] &&
        receivedData[2] == tokenClient[0] &&
        receivedData[3] == tokenClient[1] &&
        receivedData[4] == tokenClient[2] &&
        receivedData[5] == tokenClient[3]) {
        handshakeComplete = true;
    } else {
        pClient->disconnect();
    }
}

void handleButtonPress(uint8_t pin, uint8_t command) {
    if (digitalRead(pin) == LOW && (millis()-lastButtonPress>1500)) {
        lastButtonPress = millis();
        display.ssd1306_command(SSD1306_DISPLAYON);
        if(command!=0x00){
          sendData[0] = 0x21;
          sendData[1] = 0xD5;
          sendData[2] = tokenClient[0];
          sendData[3] = tokenClient[1];
          sendData[4] = tokenClient[2];
          sendData[5] = tokenClient[3];
          sendData[6] = tokenServer[0];
          sendData[7] = tokenServer[1];
          sendData[8] = tokenServer[2];
          sendData[9] = tokenServer[3];
          sendData[10] = counter++;
          sendData[11] = random(0, 255);
           sendData[12] = command ;
          sendData[13] = 0x00;
          sendData[14] = 0x00;
          sendData[15] = checksum(sendData);
          encrypt(sendData, sessionKey, encryptedMessage);
          characteristicSend->writeValue(encryptedMessage, 16);
          tm = millis();
        }
    }
}

void connectToServer() {
    if (pClient->connect(*pServerAddress)) {
        connected = true;
        BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));

        if (pRemoteService == nullptr) {
            return;
            pClient->disconnect();
        }
        characteristicSend = pRemoteService->getCharacteristic(BLEUUID(TARGET_CHARACTERISTIC_UUID_SEND));
        characteristicReceive = pRemoteService->getCharacteristic(BLEUUID(TARGET_CHARACTERISTIC_UUID_RECEIVE));

        if (characteristicReceive) {
            characteristicReceive->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
                if (handshakeComplete) {
                    decrypt(pBLERemoteCharacteristic->readRawData(), sessionKey, receivedData);

                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setCursor(0, 0);
                    display.print("Voltage: ");
                    display.println(receivedData[12]);
                    display.print("Temp: ");
                    display.println(receivedData[13] - 50);

                    if ((receivedData[14] & 0x08) != 0)
                        display.println("In gear!");
                    else
                        display.println("Not in gear");

                    if ((receivedData[14] & 0x01) != 0)
                        display.println("Lights on");
                    else
                        display.println("Lights off");

                    if ((receivedData[14] & 0x02) != 0)
                        display.println("Engine on");
                    else
                        display.println("Engine off");

                    if ((receivedData[14] & 0x04) != 0)
                        display.println("Locked");
                    else
                        display.println("Unlocked");
                }
            });
        }
    }
}

void setup() {
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallbacks());
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    tm = millis();

    display.begin(SSD1306_I2C_ADRESS, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Initializing...");
    display.display();

    pinMode(ENGINE_BTN, INPUT_PULLUP);
    pinMode(LIGHTS_BTN, INPUT_PULLUP);
    pinMode(SCREEN_BTN, INPUT_PULLUP);
    pinMode(UNLOCK_BTN, INPUT_PULLUP);
    pinMode(LOCKRIO_BTN, INPUT_PULLUP);
}

void loop() {
    if (millis() - lastButtonPress > OLED_TIMEOUT) {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }

    if (!connected && pServerAddress == nullptr && !scanning) {
        pBLEScan->start(2, false);
        scanning = true;
        tm = millis();
    }
    if (scanning && (millis() - tm > 2000)) scanning = false;

    if (!connected && pServerAddress != nullptr) 
        connectToServer();
    if (connected && !handshakeComplete)
        performHandshake();
    if (connected && handshakeComplete) {
      handleButtonPress(ENGINE_BTN, 0x1B);
      handleButtonPress(LIGHTS_BTN, 0xAA);
      handleButtonPress(SCREEN_BTN, 0x00);
      handleButtonPress(UNLOCK_BTN, 0x47);
      handleButtonPress(LOCKRIO_BTN, 0xFC);
        if (millis() - tm > 700){
          sendData[0] = 0x21;
          sendData[1] = 0xD5;
          sendData[2] = tokenClient[0];
          sendData[3] = tokenClient[1];
          sendData[4] = tokenClient[2];
          sendData[5] = tokenClient[3];
          sendData[6] = tokenServer[0];
          sendData[7] = tokenServer[1];
          sendData[8] = tokenServer[2];
          sendData[9] = tokenServer[3];
          sendData[10] = counter++;
          sendData[11] = random(0, 255);
           sendData[12] = 0xA4;
          sendData[13] = 0x00;
          sendData[14] = 0x00;
          sendData[15] = checksum(sendData);
          encrypt(sendData, sessionKey, encryptedMessage);
          characteristicSend->writeValue(encryptedMessage, 16);
          tm = millis();
        }
    }
}
