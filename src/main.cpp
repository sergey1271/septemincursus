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

const int REGIM=1;

//ПИНЫ ДЛЯ RFID-МОДУЛЯ
const int SS_PIN   = 5; 
const int SCK_PIN  = 18; 
const int MOSI_PIN = 23;
const int MISO_PIN = 19;  
const int RST_PIN  = 22;   

MFRC522 rfid(SS_PIN, RST_PIN);

const int R_PIN = 13;
const int G_PIN = 16;
const int B_PIN = 17;

//ПИНЫ ДЛЯ МОТОРОВ
const int MOTOR_A_PWM = 33;
const int MOTOR_A_1   = 26;
const int MOTOR_A_2   = 25;
const int MOTOR_B_PWM = 32;
const int MOTOR_B_1   = 14;
const int MOTOR_B_2   = 12;

int targetA = 0, currentA = 0;
int targetB = 0, currentB = 0;
unsigned long lastMotorUpdate = 0;
const int motorStepDelay = 30;
unsigned long lastServoUpdate = 0;
const int servoStepDelay = 10;

//ПИНЫ ДЛЯ СЕРВОПРИВОДОВ
const int SERVO_VERTICAL =   2;
const int SERVO_HORIZONTAL = 4;

BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Сервоприводы
Servo verticalServo;
Servo horizontalServo;
int verticalPos = 0;
int horizontalPos = 0;
const int VERTICAL_STEP_SIZE = 20;
const int HORIZONTAL_STEP_SIZE = 15;
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 63;

bool movingUp, movingDown, movingLeft, movingRight;

void processCommand(String command);
void setMotorSpeeds(int left = 100, int right = 100); 
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
       
       targetA = 0; currentA = 0;
       targetB = 0; currentB = 0;
       digitalWrite(MOTOR_A_1, LOW); digitalWrite(MOTOR_A_2, LOW); analogWrite(MOTOR_A_PWM, 0);
       digitalWrite(MOTOR_B_1, LOW); digitalWrite(MOTOR_B_2, LOW); analogWrite(MOTOR_B_PWM, 0);
        
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
    if (left >= -10 && left <= 10 && right >= -10 && right <= 10) {
        targetA = left;
        targetB = right;
    }
    if (millis() - lastMotorUpdate >= motorStepDelay) {
        lastMotorUpdate = millis();

        if (currentA < targetA) currentA++;
        else if (currentA > targetA) currentA--;

        if (currentB < targetB) currentB++;
        else if (currentB > targetB) currentB--;

        if (currentA > 0) {
            digitalWrite(MOTOR_A_1, LOW);
            digitalWrite(MOTOR_A_2, HIGH);
            analogWrite(MOTOR_A_PWM, map(currentA, 0, 10, 0, 200));
        } else if (currentA < 0) {
            digitalWrite(MOTOR_A_1, HIGH);
            digitalWrite(MOTOR_A_2, LOW);
            analogWrite(MOTOR_A_PWM, map(-currentA, 0, 10, 0, 200));
        } else {
            digitalWrite(MOTOR_A_1, LOW);
            digitalWrite(MOTOR_A_2, LOW);
            analogWrite(MOTOR_A_PWM, 0);
        }
        
        if (currentB> 0) {
            digitalWrite(MOTOR_B_1, LOW);
            digitalWrite(MOTOR_B_2, HIGH);
            analogWrite(MOTOR_B_PWM, map(currentB, 0, 10, 0, 200));
        } else if (currentB < 0) {
            digitalWrite(MOTOR_B_1, HIGH);
            digitalWrite(MOTOR_B_2, LOW);
            analogWrite(MOTOR_B_PWM, map(-currentB, 0, 10, 0, 200));
        } else {
            digitalWrite(MOTOR_B_1, LOW);
            digitalWrite(MOTOR_B_2, LOW);
            analogWrite(MOTOR_B_PWM, 0);
        }
    }
}

void setServoSpeeds(int vert, int horz) {
    if (vert == 1) {movingUp = true; movingDown = false;
    } else if (vert == -1) {movingDown = true; movingUp = false;
    } else {movingUp = false; movingDown = false;}

    if (horz == 1) { movingLeft = true; movingRight = false;
    } else if (horz == -1) { movingRight = true; movingLeft = false;
    } else { movingLeft = false; movingRight = false;}
}

void updateServos() {
    if (movingUp) {
        // verticalPos = min(verticalPos + STEP_SIZE, MAX_ANGLE);
        verticalServo.write(180 - VERTICAL_STEP_SIZE);
    }
    else if (movingDown) {
        // verticalPos = max(verticalPos - STEP_SIZE, MIN_ANGLE);
        verticalServo.write(0 + VERTICAL_STEP_SIZE);
    }
    else {
        verticalServo.write(90);
    }
    if (REGIM==0){
        if (movingLeft && horizontalPos < MAX_ANGLE) {
            horizontalPos = min(horizontalPos + HORIZONTAL_STEP_SIZE, MAX_ANGLE);
            horizontalServo.write(horizontalPos);
        }
        else if (movingRight && horizontalPos > MIN_ANGLE) {
            horizontalPos = max(horizontalPos - HORIZONTAL_STEP_SIZE, MIN_ANGLE);
            horizontalServo.write(horizontalPos);
        }
    }
    else if (REGIM==1)
    {
        // при переключении режимов поменять константу HORIZONTAL_STEP_SIZE!
        if (movingLeft) {
            horizontalServo.write(180 - HORIZONTAL_STEP_SIZE);
        }
        else if (movingRight) {
            horizontalServo.write(0 + HORIZONTAL_STEP_SIZE);
        }
        else {
            horizontalServo.write(90);
        }
    }
} 


void processCommand(String command) {
    int lIdx = command.indexOf("L:");
    int rIdx = command.indexOf(",R:");
    int vIdx = command.indexOf(",V:");
    int hIdx = command.indexOf(",H:");
    
    if (lIdx >= 0 && rIdx > 0 && vIdx > 0 && hIdx > 0) {
        int left   = command.substring(lIdx+2, rIdx).toInt();
        int right  = command.substring(rIdx+3, vIdx).toInt();
        int vert   = command.substring(vIdx+3, hIdx).toInt();
        int horz   = command.substring(hIdx+3).toInt();
        
        left  = constrain(left, -10, 10);
        right = constrain(right, -10, 10);
        vert  = constrain(vert, -1, 1);
        horz  = constrain(horz, -1, 1);
        
        setMotorSpeeds(left, right);
        setServoSpeeds(vert, horz);
        return;
    }
}

void setColor(int r, int g, int b) {
  analogWrite(R_PIN, r);
  analogWrite(G_PIN, g);
  analogWrite(B_PIN, b);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN); 
  
    rfid.PCD_Init();

    pinMode(R_PIN, OUTPUT);
    pinMode(G_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);
    setColor(0, 0, 0);
  
    byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
    if (v == 0x00 || v == 0xFF) {
        Serial.println(F("Модуль не обнаружен"));
    } else {
        Serial.println(F("RC522 найден"));
    }
    
    verticalServo.attach(SERVO_VERTICAL);
    horizontalServo.attach(SERVO_HORIZONTAL);

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
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE_NR | 
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY
    );
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
    
    setMotorSpeeds(); 
    
    delay(1);
    if ( ! rfid.PICC_IsNewCardPresent() || ! rfid.PICC_ReadCardSerial()) {
        return;
    }

    String nfcText = ""; 
    byte pagesToRead[] = {4, 8}; 
    for (int i = 0; i < 2; i++) {
      byte pageAddr = pagesToRead[i];
      byte buffer[18]; 
      byte size = sizeof(buffer);

      if (rfid.MIFARE_Read(pageAddr, buffer, &size) == MFRC522::STATUS_OK) {
        for (byte j = 0; j < 16; j++) {
          if (buffer[j] >= 32 && buffer[j] <= 126) {
            nfcText += (char)buffer[j];
          }
        }
      }
    }  
    if (nfcText.length() > 3) {
      nfcText = nfcText.substring(4);
      nfcText.trim();
      nfcText.toLowerCase();

      Serial.print(F("Распознан цвет: "));
      Serial.println(nfcText);

      if (deviceConnected && pCharacteristic != nullptr) {
          pCharacteristic->setValue(nfcText.c_str());
          pCharacteristic->notify();
      }

      // Базовые цвета
      if (nfcText == "red" || nfcText == "красный")          setColor(255, 0, 0);
      else if (nfcText == "green" || nfcText == "зеленый" || nfcText == "зелёный")   setColor(0, 255, 0);
      else if (nfcText == "blue" || nfcText == "синий")    setColor(0, 0, 255);
      else if (nfcText == "yellow" || nfcText == "желтый" || nfcText == "жёлтый")  setColor(255, 255, 0);
      else if (nfcText == "white" || nfcText == "белый")   setColor(255, 255, 255);
      else if (nfcText == "black" || nfcText == "off" || nfcText == "черный" || nfcText == "чёрный") setColor(0, 0, 0);

      else if (nfcText == "orange"  || nfcText == "оранжевый")  setColor(255, 20, 0);   // Красный на максимум, немного зеленого
      else if (nfcText == "pink" || nfcText == "розовый")    setColor(255, 20, 100);  // Красный, немного синего и зеленого для нежности
      else if (nfcText == "purple" || nfcText == "фиолетовый")  setColor(128, 0, 128);   // Половина красного, половина синего
      else if (nfcText == "brown" || nfcText == "коричневый")   setColor(150, 50, 0);    // Темно-оранжевый (ближайшее к коричневому)
      else if (nfcText == "grey" || nfcText == "gray" || nfcText == "серый") setColor(30, 30, 30); // Тусклый белый
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}