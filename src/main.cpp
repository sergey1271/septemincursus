#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

#define SERVICE_UUID        "e3924aeb-5732-4cb9-9058-46ff8ffc686a"
#define CHARACTERISTIC_UUID "9bfb5585-ee07-4bdf-b32d-5181fdb54c0a"
#define DEVICE_NAME         "ESP32_Car_7inc"

//ПИНЫ ДЛЯ RFID-МОДУЛЯ
const int SS_PIN   = 21; 
const int RST_PIN  = 4;   
const int SCK_PIN  = 18; 
const int MISO_PIN = 19;  
const int MOSI_PIN = 23; 

MFRC522 rfid(SS_PIN, RST_PIN);

//ПИНЫ ДЛЯ МОТОРОВ
const int MOTOR_A_PWM = 33;
const int MOTOR_A_1   = 26;
const int MOTOR_A_2   = 25;
const int MOTOR_B_PWM = 32;
const int MOTOR_B_1   = 14;
const int MOTOR_B_2   = 12;

//ПИНЫ ДЛЯ СЕРВОПРИВОДОВ
const int SERVO_VERTICAL = 16;
const int SERVO_HORIZONTAL = 17;

BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Сервоприводы
Servo verticalServo;
Servo horizontalServo;
int verticalPos = 0;
int horizontalPos = 0;
const int STEP_SIZE = 7;
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;

bool movingUp = false;
bool movingDown = false;
bool movingLeft = false;
bool movingRight = false;

void processCommand(String command);
void setMotorSpeeds(int left, int right);
void updateServos();


class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("[BLE] Телефон подключен");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("[BLE] Телефон отключен");
        
        movingUp = false;
        movingDown = false;
        movingLeft = false;
        movingRight = false;
        setMotorSpeeds(0, 0);
        
        pServer->getAdvertising()->start();
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        if (value.length() > 0) {
            String command = String(value.c_str());
            processCommand(command);
        }
    }
};

void setMotorSpeeds(int left, int right) {
    if (left > 0) {
        digitalWrite(MOTOR_A_1, LOW);
        digitalWrite(MOTOR_A_2, HIGH);
        analogWrite(MOTOR_A_PWM, map(left, 0, 10, 0, 255));
    } else if (left < 0) {
        digitalWrite(MOTOR_A_1, HIGH);
        digitalWrite(MOTOR_A_2, LOW);
        analogWrite(MOTOR_A_PWM, map(-left, 0, 10, 0, 255));
    } else {
        digitalWrite(MOTOR_A_1, LOW);
        digitalWrite(MOTOR_A_2, LOW);
        analogWrite(MOTOR_A_PWM, 0);
        Serial.println("[Мотор] Левый остановлен");
    }
    
    if (right > 0) {
        digitalWrite(MOTOR_B_1, LOW);
        digitalWrite(MOTOR_B_2, HIGH);
        analogWrite(MOTOR_B_PWM, map(right, 0, 10, 0, 255));
    } else if (right < 0) {
        digitalWrite(MOTOR_B_1, HIGH);
        digitalWrite(MOTOR_B_2, LOW);
        analogWrite(MOTOR_B_PWM, map(-right, 0, 10, 0, 255));
    } else {
        digitalWrite(MOTOR_B_1, LOW);
        digitalWrite(MOTOR_B_2, LOW);
        analogWrite(MOTOR_B_PWM, 0);
        Serial.println("[Мотор] Правый остановлен");
    }
}

void updateServos() {
    if (movingUp && verticalPos < MAX_ANGLE) {
        verticalPos = min(verticalPos + STEP_SIZE, MAX_ANGLE);
        verticalServo.write(verticalPos);
    }
    else if (movingDown && verticalPos > MIN_ANGLE) {
        verticalPos = max(verticalPos - STEP_SIZE, MIN_ANGLE);
        verticalServo.write(verticalPos);
    }
    
    if (movingLeft && horizontalPos < MAX_ANGLE) {
        horizontalPos = min(horizontalPos + STEP_SIZE, MAX_ANGLE);
        horizontalServo.write(horizontalPos);
    }
    else if (movingRight && horizontalPos > MIN_ANGLE) {
        horizontalPos = max(horizontalPos - STEP_SIZE, MIN_ANGLE);
        horizontalServo.write(horizontalPos);
    }
}

void processCommand(String command) {
    if (command.startsWith("L:") && command.indexOf(",R:") > 0) {
        int lIndex = command.indexOf("L:") + 2;
        int commaIndex = command.indexOf(",R:");
        int rIndex = commaIndex + 3;
        
        if (lIndex > 1 && commaIndex > 0 && rIndex > 0) {
            String leftStr = command.substring(lIndex, commaIndex);
            String rightStr = command.substring(rIndex);
            
            int leftSpeed = leftStr.toInt();
            int rightSpeed = rightStr.toInt();
            
            setMotorSpeeds(leftSpeed, rightSpeed);
            Serial.print("[Моторы] L:");
            Serial.print(leftSpeed);
            Serial.print(" R:");
            Serial.println(rightSpeed);
        }
        return;
    }
  
    if (command == "UPv") {
        movingUp = true;
        movingDown = false;
        Serial.println("[Серво] Вертик вверх");
        return;
    }
    if (command == "UPv_STOP") {
        movingUp = false;
        return;
    }
    
    if (command == "DOWNv") {
        movingDown = true;
        movingUp = false;
        Serial.println("[Серво] Вертик вниз");
        return;
    }
    if (command == "DOWNv_STOP") {
        movingDown = false;
        return;
    }
    
    if (command == "UPg") {
        movingLeft = true;
        movingRight = false;
        Serial.println("[Серво] Гориз вверх");
        return;
    }
    if (command == "UPg_STOP") {
        movingLeft = false;
        return;
    }
    
    if (command == "DOWNg") {
        movingRight = true;
        movingLeft = false;
        Serial.println("[Серво] Гориз вниз");
        return;
    }
    if (command == "DOWNg_STOP") {
        movingRight = false;
        return;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN); 
  
    rfid.PCD_Init();
  
    byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
    Serial.print(F("Версия чипа: 0x"));
    Serial.println(v, HEX);
  
    if (v == 0x00 || v == 0xFF) {
        Serial.println(F("Модуль не обнаружен"));
    } else {
        Serial.println(F("RC522 найден"));
    }
    
    verticalServo.attach(SERVO_VERTICAL);
    horizontalServo.attach(SERVO_HORIZONTAL);
    verticalServo.write(verticalPos);
    horizontalServo.write(horizontalPos);

    pinMode(MOTOR_A_PWM, OUTPUT);
    pinMode(MOTOR_A_1, OUTPUT);
    pinMode(MOTOR_A_2, OUTPUT);
    pinMode(MOTOR_B_PWM, OUTPUT);
    pinMode(MOTOR_B_1, OUTPUT);
    pinMode(MOTOR_B_2, OUTPUT);

    Serial.println("\n=== ESP32 Car Controller v2.0 ===");
    
    BLEDevice::init(DEVICE_NAME);
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_READ);
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}

void loop() {
    if (deviceConnected) {
        updateServos();
    }
    delay(10);

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i], HEX);
        }
        Serial.print("UID карты: ");
        Serial.println(uid);
        
        // ДОБАВЬТЕ ОБРАБОТКУ КАРТЫ
        if (uid == "ab12cd34") {  // Замените на ваш UID
            setMotorSpeeds(10, 10);  // Едем вперед
            delay(2000);
            setMotorSpeeds(0, 0);    // Останавливаемся
        }
        
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}