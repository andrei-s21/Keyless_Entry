#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "mbedtls/aes.h"
#include <stdio.h>
#include<string>
#include "esp_log.h"
#include "sdkconfig.h"
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_gatt_common_api.h>
#include "BLEService.h"
#include "GeneralUtils.h"
#include <sstream>
#include <unordered_set>
#include "esp32-hal-log.h"

#define SERVICE_UUID        "5daea2b4-1912-4839-8c9d-7657f074fa84"
#define CHARACTERISTIC_UUID_SEND "df847cb0-69aa-43e1-8aa9-1fad496c60b5"//send
#define CHARACTERISTIC_UUID_RECEIVE "f61ad6ec-247b-44e9-98e1-f2a61b9836ea"//receive
#define SERIAL1_TX 17
#define SERIAL1_RX 16

///////////////////////////////////////////////////////////////////////////////////////////////////////// VARIABLES 

SemaphoreHandle_t serialMutex;
BLEScan* pBLEScan;
BLEServer *server;
BLEService *service;
BLECharacteristic *caracteristic;
BLECharacteristic *caracteristicRecive;

bool isDeviceConnected=false;
bool availableSessionKey=false;
bool intruderEjected=false;

uint8_t output[16]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t outSend[16]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t masterKey [16]={0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00};// replace with your master key , same on client
uint8_t sessionKey [16]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t iv [16]={0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00};// replace with your iv , same on client

///////////////////////////////////////////////////////////////////////////////////////////////////////// ENCRIPTION

void encrypt( uint8_t *input, uint8_t *key, uint8_t *iv , uint8_t *output) 
{
  uint8_t key_copy[16] = {0};
  uint8_t iv_copy[16] = {0}; 

  memcpy(key_copy, key, 16);
  memcpy(iv_copy, iv, 16);

  mbedtls_aes_context aes; 
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key_copy, 128);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, 16, iv_copy,(unsigned char *)input, output);

}

void decrypt(uint8_t*input,  const unsigned char *key,  uint8_t *iv, uint8_t* dec)
{
    
  uint8_t key_copy[16] = {0};
  uint8_t iv_copy[16] = {0};

  memcpy(key_copy, key, 16);
  memcpy(iv_copy, iv, 16);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, key_copy, 128);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 16, iv_copy, (unsigned char *)input, dec);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////// CALLBACKS

static void my_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  if(xSemaphoreTake(serialMutex, portMAX_DELAY))
    {
      output[13]=(byte)(-param->read_rssi_cmpl.rssi);
      
      for(short i=0;i<16;i++)
      {
        Serial1.write(output[i]);
        output[i]=0x00;
      }
       
      xSemaphoreGive(serialMutex);
    }  
}

class MyCharacteristicCallback:public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic* data,esp_ble_gatts_cb_param_t* param){     

    if(availableSessionKey)
    {
      decrypt(data->getData(),sessionKey,iv,output);  
      ::esp_ble_gap_read_rssi(param->read.bda);
    }
    else 
    {
      if(xSemaphoreTake(serialMutex,portMAX_DELAY))
      {
        decrypt(data->getData(),masterKey,iv,sessionKey);
        availableSessionKey=true;
        output[0]=0x96;
        output[1]=0xAC;
        for(short i=0;i<16;i++) {
          Serial1.write(output[i]);
          output[i]=0x00;
        }
        xSemaphoreGive(serialMutex);
      }
    }
  }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
      if(!isDeviceConnected){
        if(xSemaphoreTake(serialMutex, portMAX_DELAY))
        {
          availableSessionKey=false;
          isDeviceConnected=true;
          output[0]=0x96;
          output[1]=0x5A;
          for(short i=0;i<16;i++){
            Serial1.write(output[i]);
            output[i]=0x00;
          }
          xSemaphoreGive(serialMutex);
        }
      }else{
        intruderEjected=true;
        server->disconnect(server->getConnId());
      }
    }; 

    void onDisconnect(BLEServer* pServer) {
      if(intruderEjected){
        intruderEjected=false;
      }else{
        if(xSemaphoreTake(serialMutex, portMAX_DELAY))
        {
          availableSessionKey=false;
          isDeviceConnected=false;
          output[0]=0x96;
          output[1]=0xF0;
          for(short i=0;i<16;i++){
            Serial.write(output[i]);
            sessionKey[i]=0x00;
            output[i]=0x00;
            outSend[i]=0x00;
          }
          xSemaphoreGive(serialMutex);
        }  
      BLEDevice::startAdvertising();
      }
    }    
};

void handleArduinoRequest(){
  if(output[1]==0x49)
    server->disconnect(server->getConnId());
}

void startServer(){
  BLEDevice::init("ESP32");
  BLEDevice::setCustomGapHandler(my_gap_event_handler);
  
  server=BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());
  
  service=server->createService(BLEUUID(SERVICE_UUID) , 32);
  caracteristic = service->createCharacteristic(CHARACTERISTIC_UUID_SEND, BLECharacteristic::PROPERTY_READ |
                  BLECharacteristic::PROPERTY_WRITE|
                  BLECharacteristic::PROPERTY_NOTIFY |
                  BLECharacteristic::PROPERTY_INDICATE);
  caracteristic->addDescriptor(new BLE2902());
  delay(2000);
  caracteristicRecive=service->createCharacteristic(CHARACTERISTIC_UUID_RECEIVE , BLECharacteristic::PROPERTY_READ |
                  BLECharacteristic::PROPERTY_WRITE|
                  BLECharacteristic::PROPERTY_NOTIFY |
                  BLECharacteristic::PROPERTY_INDICATE);
  BLE2902 * desc= new BLE2902();
  desc->setNotifications(true);
  caracteristicRecive->addDescriptor(desc); 
  caracteristicRecive->setCallbacks(new MyCharacteristicCallback());

  service->start();

  BLEDevice::startAdvertising();
}

void setup() {
  serialMutex=xSemaphoreCreateMutex();
  Serial1.begin(115200,SERIAL_8N1,SERIAL1_RX,SERIAL1_TX);
  startServer();
}

void loop() {
  if(Serial1.available()>15){
    if(xSemaphoreTake(serialMutex, portMAX_DELAY))
    {   
      for(short i=0;i<16;i++)
        output[i]=Serial1.read();
      if(output[0]==0xA6) handleArduinoRequest();
      else if(output[0]==0xF2 && availableSessionKey)
      {
        encrypt(output,sessionKey,iv,outSend);
        caracteristic->setValue(outSend,16);
        caracteristic->notify(true);
        for(short i=0;i<16;i++)
        {
           output[i]=0x00;
           outSend[i]=0x00;
        }
      }
      xSemaphoreGive(serialMutex);
    }
  }
}
