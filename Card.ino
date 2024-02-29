#include <esp_now.h>
#include <WiFi.h>
// run this code to know MacAddress
 
#ifdef ESP32#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  5  // ESP32 pin GPIO5 
#define RST_PIN 27 // ESP32 pin GPIO27 

MFRC522 rfid(SS_PIN, RST_PIN);

//ขั้นตอนการลงทะเบียน กดฟังชั่นสัมติง -> เเตะบัตรที่ต้องการ -> ทำการเข้าสู่โหมดลงทะเบียน
int mode = 3;
uint8_t MacAddress1[] = {0xE8, 0x68, 0xE7, 0x23, 0x82, 0x1C}; //led
uint8_t MacAddress2[] = {0x24, 0x6F, 0x28, 0x28, 0x15, 0x94}; //Oled
uint8_t MacAddress3[] = {0x3C, 0x61, 0x05, 0x03, 0xCA, 0x04}; //Pin
uint8_t MacAddress4[] = {0x24, 0x6F, 0x28, 0x28, 0x17, 0x1C}; //Servo

int oled_tap_card = 0;
int mode0 = 0;
int mode1 = 0;
struct RFIDTag {    
  char uid[15];
  char *name;
  int stade;
  int pass;
};

RFIDTag tags[] = {  //ข้อมูลพนักงงาน เปลี่ยนให้เป็นตามบัตรเรา
  {"76fb9749", "player 1",0,NULL}, // อาจจะต้องเปลี่ยนวิธีการบันทึกข้อมูล
  {"834bb9fa", "player 2",0,NULL},
  {"03aa091b", "player 3",0,NULL},
  {"86774f49", "player 4",0,NULL},
  {"63e68ffb", "player 5",0,NULL},
  {"860c1e49", "player 6",0,NULL},
};
// 0(ปิด) 1(กำลังลงทะเบียน) 2(ลงทะเบียนเเล้ว)
byte totalTags = sizeof(tags) / sizeof(RFIDTag);

///ฝั่งชั่นการลงทะเบียน ต้องเปลี่ยน stade

void changeTagStatus(const char* uid, int newStatus) {
  for (int i = 0; i < totalTags; i++) {
    if (strcmp(uid, tags[i].uid) == 0) {
      tags[i].stade = newStatus;
      return; // Exit the function after updating the status
    }
  }

  // มีการส่งค่า newstatus ออกไปข้างนอกด้วย  
} // กดปุ่มเเล้วทริกเกอร์บัดรให้ทำงาน 

int checkStatus(String uid){
  int check = 0;
  for(int i = 0 ; i <totalTags ; i++){
    if( uid == tags[i].uid && tags[i].stade == 2){
      return 1;
    }else if(tags[i].stade == 2){
      check = 3;
    }
  }
  return check;
}
void clearstatus(const char* uid){
  for (int i = 0; i < totalTags; i++){
    if (strcmp(uid, tags[i].uid) == 0) {
      tags[i].stade = 0 ;
      tags[i].pass = 0 ; // ?
      return;
    }
  }
}

void changepass(const char* uid, int pass){
  for (int i = 0; i < totalTags; i++){
    if (strcmp(uid, tags[i].uid) == 0) {
      tags[i].pass = pass ; // ?
      return;

    }
  }
}
bool compareMac(const uint8_t * a, uint8_t * b){
  for(int i=0;i<6;i++){
    if(a[i]!=b[i])
      return false;    
  }
  return true;
}
// ================================ ส่งข้อมูล ================================
typedef struct send_mode1{
  int statuss;
} send_mode1;

typedef struct send_mode{
  int statuss; // 0 regis 1 reset 2 forget card 3 reset complete 4 register complete
}send_mode;

typedef struct send_servo1{
  int statuss;
}send_servo1;

typedef struct send_oled{
  int statuss;
}send_oled;



typedef struct servo_struct{
  int servo_status;//1 = lock, 0 = unlock
} servo_struct;

send_mode1 test;
send_mode1 led;
send_servo1 servo;
send_oled oled;
send_mode1 pin;
send_mode repin;
esp_now_peer_info_t peerInfo1; // Led
esp_now_peer_info_t peerInfo2; // Olde
esp_now_peer_info_t peerInfo3; // Pin
esp_now_peer_info_t peerInfo4; // Servo
 // จำนวนเเท้ก  

/* void addTag(const char* uid, const char* name) {
  if (totalTags < sizeof(tags) / sizeof(RFIDTag)) {
    strcpy(tags[totalTags].uid, uid);
    strcpy(tags[totalTags].name, name);
    totalTags++;
  }
} */

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  //receive data
  if(compareMac(mac_addr,MacAddress3)){
    memcpy(&repin, incomingData, sizeof(repin));
    if(repin.statuss == 0){
      Serial.println("register");
      mode = 2;
    }else if(repin.statuss == 1){
      Serial.println("reste");
      mode = 1;
    }else if(repin.statuss == 2){
      Serial.print("forget card");
      mode = 0;
    }else if(repin.statuss == 3){
      Serial.println("reset Complete");
      mode = 3;
    }else if(repin.statuss == 4){
      Serial.println("register Complete");
      oled_tap_card = 0;
      mode = 3;
    }else if(repin.statuss == 5){
      Serial.println("Open");
      mode = 0;
      }
  }
  if(compareMac(mac_addr,MacAddress4)){
     Serial.println("Close");
     mode = 3;
  }
}

void setup() {
  int mode = 0 ;
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("โปรดทำการเเตะบัตร");
// ======================================== ส่งข้อมูล ==================================
  WiFi.mode(WIFI_STA);
 
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(peerInfo1.peer_addr, MacAddress1, 6);
  peerInfo1.channel = 0;  
  peerInfo1.encrypt = false;
  if (esp_now_add_peer(&peerInfo1) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo2.peer_addr, MacAddress2, 6);
  peerInfo2.channel = 0;  
  peerInfo2.encrypt = false;
  if (esp_now_add_peer(&peerInfo2) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo3.peer_addr, MacAddress3, 6);
  peerInfo3.channel = 0;  
  peerInfo3.encrypt = false;
  if (esp_now_add_peer(&peerInfo3) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  memcpy(peerInfo4.peer_addr, MacAddress4, 6);
  peerInfo4.channel = 0;  
  peerInfo4.encrypt = false;
  if (esp_now_add_peer(&peerInfo4) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  
  if(mode == 0){//oFF รอ Status Choon
    if(mode0 == 0){
      Serial.println("mode0");
      Serial.println("mode0");
      mode0 = 1;
      mode1 = 1;
      oled_tap_card = 0;
    }
   }
    
  if(mode == 1 ){
    mode0 = 0;
    if(mode1 == 0){
      oled.statuss = 5;
      esp_err_t result1 = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(oled));
      if (result1 == ESP_OK ){
           Serial.println("Sent with success");
           mode1 = 1;
          }
      else {
           Serial.println("Error sending the data");
           }
           delay(2000);
    }
      if (rfid.PICC_IsNewCardPresent()) { // new tag is available
        if (rfid.PICC_ReadCardSerial()) { // NUID has been readed

          MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
          Serial.print("RFID/NFC Tag Type: ");
          Serial.println(rfid.PICC_GetTypeName(piccType));
          String uidString = "";
          for (int i = 0; i < rfid.uid.size; i++) {
            if (rfid.uid.uidByte[i] < 0x10) {
            uidString += "0"; // Add a leading zero if necessary
            }
            Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(rfid.uid.uidByte[i], HEX);
  
            uidString += String(rfid.uid.uidByte[i], HEX);
        }
        Serial.println(uidString);
        int Status = checkStatus(uidString);
        if(Status == 0){
          Serial.println("Pls Register");
          oled.statuss = 0;
          esp_err_t result = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(oled));
          if (result == ESP_OK) {
              Serial.println("Sent with success");
            }
            else {
              Serial.println("Error sending the data");
            }
              delay(2000);
        }else if(Status == 1){
          clearstatus(uidString.c_str());
          pin.statuss = 1;
          esp_err_t result1 = esp_now_send(MacAddress3, (uint8_t *) &pin, sizeof(send_mode1));
          if (result1 == ESP_OK) {
              Serial.println("Sent with success");
              }
          else{
               Serial.println("Error sending the data");
               }
            delay(500);
            mode = 0;
        }else if(Status == 3){
          Serial.print("You Not Owner");
          oled.statuss = 1;
          esp_err_t result = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(oled));
          if (result == ESP_OK) {
              Serial.println("Sent with success");
            }
            else {
              Serial.println("Error sending the data");
            }
            delay(500);
            mode = 1;
          }
        rfid.PICC_HaltA(); // halt PICC
        rfid.PCD_StopCrypto1(); // stop encryption on PCD
      }
    }
  }


  if(mode == 2){ // register 
    mode0 = 0;
    mode1 = 1;
    int count = 0;
    for(int i = 0 ; i< totalTags ; i++){
      if(tags[i].stade == 2){
        count++;
      }
    }
    if( count == 0){
    if (rfid.PICC_IsNewCardPresent()) { // new tag is available
      if (rfid.PICC_ReadCardSerial()) { // NUID has been readed

        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        Serial.print("RFID/NFC Tag Type: ");
        Serial.println(rfid.PICC_GetTypeName(piccType));
        String uidString = "";
        for (int i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) {
          uidString += "0"; // Add a leading zero if necessary
          }
          Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(rfid.uid.uidByte[i], HEX);

          uidString += String(rfid.uid.uidByte[i], HEX);
        }
        Serial.println();
        uidString.replace(" ", "");
        for(int i = 0 ; i < totalTags ; i++){
          if(uidString == tags[i].uid){
            tags[i].stade =2;
            Serial.print("register Succere");
          }
        }
       pin.statuss = 1;
       esp_err_t result1 = esp_now_send(MacAddress3, (uint8_t *) &pin, sizeof(send_mode1));
       if (result1 == ESP_OK) {
           Serial.println("Sent with success");
           }
           else {
             Serial.println("Error sending the data");
            }
              delay(2000);
       mode = 0;
      }
    }
    }else{
      Serial.println("Please reset");
      oled.statuss = 6;
      esp_err_t result1 = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(oled));
      if (result1 == ESP_OK ) {
           Serial.println("Sent with success");
           }
           else {
             Serial.println("Error sending the data");
            }
              delay(2000);
  }
 }
  
  if(mode == 3){
    delay(1000);
    mode1 = 1;
    mode0 = 0;
    oled.statuss = 5;
      if(oled_tap_card == 0){
        esp_err_t result1 = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(oled));
        oled_tap_card = 1;
      }
    if (rfid.PICC_IsNewCardPresent()) { // new tag is available
      if (rfid.PICC_ReadCardSerial()) { // NUID has been readed

        oled_tap_card = 1;
        MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
        Serial.print("RFID/NFC Tag Type: ");
        Serial.println(rfid.PICC_GetTypeName(piccType));
        // print UID in Serial Monitor in the hex format

        Serial.print("UID:");
        String uidString = "";
        for (int i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) {
          uidString += "0"; // Add a leading zero if necessary
          }
          Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(rfid.uid.uidByte[i], HEX);

          uidString += String(rfid.uid.uidByte[i], HEX);
        }
        Serial.println();
      
        uidString.replace(" ", ""); // Remove spaces from uidString
        
/*    Serial.print(uidString);
      Serial.println();
      Serial.println(tags[1].uid); */


        bool they_match = false;
        int countcard = 0;
      
        for(int i = 0 ; i < totalTags ; i++){

          if(tags[i].stade == 2 ){//บอกว่าบัตรได้ทำการลงทะเบียนเเล้ว เเละรหัสถูกต้อง
            countcard++;
            if(uidString == tags[i].uid){

              Serial.println(tags[i].name);
              they_match = true;
              break;
           }
          }
        }

        if(they_match) // เเสดงข้อความ 
        {
          Serial.print(F("Access Granted!")); // ส่ง Olde
          // ================== ส่งข้อมูล ================================
//          led.statuss = 1;
          pin.statuss = 2;
          servo.statuss = 0;
          Serial.println("Please reset");
          oled.statuss = 7;
          esp_err_t result1 = esp_now_send(MacAddress3, (uint8_t *) &pin, sizeof(send_mode1)); // pin off
           if (result1 == ESP_OK){
           Serial.println("Sent with success pin");
           }
           else {
             Serial.println("Error sending the data");
            }
              delay(500);
          esp_err_t result2 = esp_now_send(MacAddress4, (uint8_t *) &servo, sizeof(send_servo1)); // servo open
           if (result2 == ESP_OK){
           Serial.println("Sent with success servo");
           }
           else {
             Serial.println("Error sending the data servo");
            }
              delay(500);
          esp_err_t result3 = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(send_mode1));// oled
           if (result3 == ESP_OK){
           Serial.println("Sent with success oled");
           }
           else {
             Serial.println("Error sending the data oled");
            }
              delay(2000);
          mode = 0;  
        }else if(countcard == 0){
          Serial.println("register first");
          oled.statuss = 0;
          esp_err_t result = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(oled));
          if (result == ESP_OK) {
              Serial.println("Sent with success");
            }
            else {
              Serial.println("Error sending the data");
            }
              delay(500);
          }
        else{ 
          oled.statuss = 8;
          esp_err_t result = esp_now_send(MacAddress2, (uint8_t *) &oled, sizeof(send_mode1));
          if (result == ESP_OK ){
           Serial.println("Sent with success");
           }
           else {
             Serial.println("Error sending the data");
            }
          delay(2000);
          Serial.print(F("Access Denied!"));
          Serial.println();
          Serial.print(F("คุณยังไม่ได้ลงทะเบียน"));
          digitalWrite(17, HIGH); 
          mode = 3;
        }
          oled_tap_card = 0;

        rfid.PICC_HaltA(); // halt PICC
        rfid.PCD_StopCrypto1(); // stop encryption on PCD
        }
      }
    }
}
 