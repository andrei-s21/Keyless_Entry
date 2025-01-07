#include <DHT11.h>
#include <SPI.h>
#include <MFRC522.h>

#define SDA 53
#define RST 5

MFRC522 rfid (SDA,RST);

DHT11 temp(26);

uint8_t data[16];
uint8_t dataToBeSent[16];
uint8_t tokenS[4]={0x00,0x00,0x00,0x00};
uint8_t tokenC[4]={0x00,0x00,0x00,0x00};
uint8_t UID[4]={0x00, 0x00,0x00,0x00}; // RFID UID
unsigned long time=0;
int attempt=0;
bool inGear=false;
bool expectedToken=true;
bool readyToSend=false;
int btn=0;
int lastBtn=0;
uint8_t counter=0;
uint8_t RSSI=100;
bool deviceConnected=false;
bool locked=false;
bool lightsOn=false;
bool ACC=false;
bool CON=false;
bool keyless=true;
bool ovverride=false;
bool firstLockSent=false ;
bool firstUnlockSent= false;
bool emergencyAccess=false;

uint8_t checksum(uint8_t * array)
{
  uint8_t count=0;
  uint8_t byteValue=0;
  for(short i=0;i<15;i++)
  {
    if (i!=13){
      byteValue=array[i];
      while(byteValue)
      {
        count+=byteValue&1;
        byteValue>>=1;
      }
    }
  }
  return count;
}


void clearBuffer(){
  while(Serial1.available()>0)
    Serial1.read();
  Serial1.flush();
}

void dissconnect(){
  Serial1.write(0xA6);
  Serial1.write(0x49);
  for(short i=0;i<14;i++)
    Serial1.write(0x00);
}

void handleRejection(){
  for(short i=0;i<16;i++)
    Serial1.write(0x00);
  ++attempt;
  if (attempt==3)
  {
    clearBuffer();
    attempt=0;
  }
}
void lights(){
  if(lightsOn){
    digitalWrite(8,LOW);
    lightsOn=false;
  }else {
    digitalWrite(8,HIGH);
    lightsOn=true;
  }
}
void lock(){
  if(!locked){
    if(lightsOn){
      digitalWrite(8,LOW);
      lightsOn=false;
    }
    digitalWrite(12,HIGH);
    delay(100);
    digitalWrite(12,LOW);
    locked=true;
  }
}
void unlock(){
  if(locked){
    if(analogRead(A0)<250&&!lightsOn){
      digitalWrite(8,HIGH);
      lightsOn=true;
    }
    digitalWrite(14,HIGH);
    delay(100);
    digitalWrite(14,LOW);
    locked=false;
  }
}
void engine(){
  if(ACC==true){
    digitalWrite(4,LOW);
    digitalWrite(2,LOW);
    ACC=false;
    CON=false;
    if(analogRead(A0)>250){
      digitalWrite(8,LOW);
      lightsOn=false;
    }
  }else
  {
    if(digitalRead(24)==LOW)
    {
      digitalWrite(2,HIGH);
      digitalWrite(4,HIGH);
      delay(1500);
      digitalWrite(6,HIGH);
      delay(1000);
      digitalWrite(6,LOW);
      ACC=true;
      CON=true;
      ovverride=false;
      if(!lightsOn)
      {
        digitalWrite(8,HIGH);
        lightsOn=true;
      }
    }
  }
}
void trunk(){
  unlock();
  digitalWrite(28,HIGH);
  delay(100);
  digitalWrite(28,LOW);
}
void key(){
  RSSI=data[13]-10;
  if(RSSI>86)
  {
    if(firstLockSent)
      if(!locked && !ovverride && keyless) lock();
    firstLockSent=true;
    firstUnlockSent=false;
  }else if(RSSI<79){
    if(firstUnlockSent)
      if(locked && !ovverride && keyless) unlock();
    firstUnlockSent=true;
    firstLockSent=false;
  }
}
void handleAction(){
  if(checksum(data)==data[15])
  {
    if((data[10]>counter||(counter==255&&data[10]==0))&&data[6]==tokenS[0]&&data[7]==tokenS[1]&&data[8]==tokenS[2]&&data[9]==tokenS[3]&&data[2]==tokenC[0]&&data[3]==tokenC[1]&&data[4]==tokenC[2]&&data[4]==tokenC[3])
    {
      ++counter;
      switch(data[12]){
        case 0xA4: key();   break; //RSSI
        case 0xAA: lights(); break; //LIGHTS
        case 0xFC: lock(); ovverride=true;  break; //LOCK
        case 0x47: unlock(); ovverride=true; break; //UNLOCK
        case 0x1B: engine(); break; //ENGINE
        case 0xB7: trunk();  break; //TRUNK
        case 0xC7: keyless=true;  break; //KEYLESS EN.
        case 0x9D: keyless=false; break; //KEYLESS DIS.
      }
    }
  }
}

void  handleClientPackage(){
  switch(data[1]){
    case 0xD5: handleAction(); break;
    case 0xB3: 
      if(checksum(data)==data[15]&&data[6]==tokenS[0]&&data[7]==tokenS[1]&&data[8]==tokenS[2]&&data[9]==tokenS[3]&&expectedToken)
      {
        expectedToken=false;
        readyToSend=true;
        tokenC[0]=data[2];
        tokenC[1]=data[3];
        tokenC[2]=data[4];
        tokenC[3]=data[5];

        dataToBeSent[0]=0xF2;
        dataToBeSent[1]=0xC2;
        dataToBeSent[2]=tokenC[0];
        dataToBeSent[3]=tokenC[1];
        dataToBeSent[4]=tokenC[2];
        dataToBeSent[5]=tokenC[3];
        dataToBeSent[11]=(byte)random(255);
        dataToBeSent[15]=checksum(dataToBeSent);
        for(short i=0;i<16;i++)
        {
          Serial1.write(dataToBeSent[i]);
          dataToBeSent[i]=0x00;
        }
        delay(500);
      }
      else dissconnect();
      break;
  }
}

void handleESPRequest()
{
  if(deviceConnected){
    if(data[1]==0xAC){// ready to start handshake
      dataToBeSent[0]=0xF2;
      dataToBeSent[1]=0xA4;
      dataToBeSent[2]=tokenS[0];
      dataToBeSent[3]=tokenS[1];
      dataToBeSent[4]=tokenS[2];
      dataToBeSent[5]=tokenS[3];
      dataToBeSent[11]=(byte)random(255);
      dataToBeSent[15]=checksum(dataToBeSent);
      for(short i=0;i<16;i++)
      {
        Serial1.write(dataToBeSent[i]);
        dataToBeSent[i]=0x00;
      }
      attempt=0;
    }
    else if (data[1]==0xF0){
      deviceConnected=false;
      attempt=0;
      counter=0;
      RSSI=100;
      ovverride=false;
      firstLockSent=false ;
      firstUnlockSent= false;
      expectedToken=true;
      readyToSend=false;
      for(uint8_t i =0;i<4;i++){
        tokenS[i]=0x00;
        tokenC[i]=0x00;
      }
      lock();
    }
    else handleRejection(); 
  }
  else{
    if(data[1]==0x5A)
    {
      deviceConnected=true;
      for(short i=0;i<4;i++)
        tokenS[i]=(byte)random(255);
      for(short j=0;j<16;j++)
        Serial1.write(0xFF);
      attempt=0;
    }
    else 
    handleRejection();
  }
}

void setup() {
  pinMode(2,OUTPUT);//ACC
  pinMode(4,OUTPUT);//CON
  pinMode(6,OUTPUT);//START
  pinMode(8,OUTPUT);//LIGHTS
  pinMode(10,INPUT);//STARTBTN
  pinMode(12,OUTPUT);//LOCK
  pinMode(14,OUTPUT);//UNLOCK
  pinMode(28,OUTPUT);//TRUNK
  pinMode(22,INPUT);//HA
  pinMode(24,INPUT);//HS
  Serial1.begin(115200);
  randomSeed(analogRead(A2));
}

void loop() {
  if(Serial1.available()>15)
  {
    for(short i=0;i<16;i++)
      data[i]=Serial1.read();
    switch(data[0]){
      case 0x96: handleESPRequest(); break;
      case 0x21: handleClientPackage(); break;
      default : handleRejection(); break;
    }
    if(readyToSend){
      dataToBeSent[0]=0xF2;

      dataToBeSent[1]=0xD5;
      dataToBeSent[2]=tokenC[0];
      dataToBeSent[3]=tokenC[1];
      dataToBeSent[4]=tokenC[2];
      dataToBeSent[5]=tokenC[3];
      dataToBeSent[6]=tokenS[0];
      dataToBeSent[7]=tokenS[1];
      dataToBeSent[8]=tokenS[2];
      dataToBeSent[9]=tokenS[3];
      dataToBeSent[10]=0x00;
      dataToBeSent[11]=(byte)random(255);
      dataToBeSent[12]=(byte)((150*analogRead(A1))/1023);
      dataToBeSent[13]=(byte)(temp.readTemperature()+50);
      if(digitalRead(24)==LOW)inGear=true;
      else inGear=false;
      dataToBeSent[14]=inGear<<4|keyless<<3|locked<<2|CON<<1|lightsOn;
      dataToBeSent[15]=checksum(dataToBeSent);
      for(short i =0;i<16;i++)
      {
        Serial1.write(dataToBeSent[i]);
        dataToBeSent[i]=0x00;
      }
    }
  }
  btn=digitalRead(10);
  if(btn==HIGH&&btn!=lastBtn&&RSSI<69&&deviceConnected||emergencyAccess)
  {
    ovverride=false;
    if(digitalRead(22)==HIGH)//clutch pressed
    {
      if(ACC==false&&CON==false)
      {
        digitalWrite(2,HIGH);
        digitalWrite(4,HIGH);
        delay(1500);
        digitalWrite(6,HIGH);
        delay(1000);
        digitalWrite(6,LOW);
        ACC=true;
        CON=true;
        if(!lightsOn)
        {
          digitalWrite(8,HIGH);
          lightsOn=true;
        }
      }else if (ACC==true&&CON==false)
      {
        digitalWrite(4,HIGH);
        delay(1500);
        digitalWrite(6,HIGH);
        delay(1000);
        digitalWrite(6,LOW);
        CON=true;
        if(!lightsOn)
        {
          digitalWrite(8,HIGH);
          lightsOn=true;
        }
      }else if (ACC==true&&CON==true)
      {
        digitalWrite(2,LOW);
        digitalWrite(4,LOW);
        digitalWrite(6,LOW);
        ACC=false;
        CON=false;
        if(analogRead(A0)>250){
          digitalWrite(8,LOW);
          lightsOn=false;
        }
      }
    }else {
      if(ACC==false&&CON==false)
      {
        digitalWrite(2,HIGH);
        ACC=true;
      }else if(ACC==true&&CON==false)
      {
        digitalWrite(2,LOW);
        ACC=false;
        if(analogRead(A0)>250){
          digitalWrite(8,LOW);
          lightsOn=false;
        }
      }else if(ACC==true&&CON==true)
      {
        digitalWrite(4,LOW);
        CON=false;
        if(lightsOn)
        {
          digitalWrite(8,LOW);
          lightsOn=false;
        }
      }
    }
  }
  lastBtn=btn;
  if(millis()-time>5000)
  {
    emergencyAccess=false;
    time=millis();
    if(rfid.PICC_IsNewCardPresent()){
      if (rfid.PICC_ReadCardSerial()) {
        if(rfid.uid.uidByte[0]==UID[0]&&rfid.uid.uidByte[1]==UID[1]&&rfid.uid.uidByte[2]==UID[2]&&rfid.uid.uidByte[3]==UID[3])
          emergencyAccess=true;
        rfid.PICC_HaltA(); 
      } 
    }
  }
}
