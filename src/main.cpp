#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Настройки BLE
BLEServer *pServer = nullptr;
BLECharacteristic *pTemperatureCharacteristic = nullptr;
BLECharacteristic *pHumidityCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

const int LEDq = 2; // Пин предустановленного на плату светодиода
unsigned long previousMillis = 0;
const long interval = 1000; // Интервал обновления показаний (мс)

// UUID для сервиса и характеристик
#define SERVICE_UUID "6f59f19e-2f39-49de-8525-5d2045f4d999"
#define HUMIDITY_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"

// ИЗ СТАРОЙ ПРОГРАММЫ
// #define SERVICE_CONTROL_UUID   "6f59f19e-2f39-49de-8525-5d2045f4d999" 
// #define SERVICE_WORK_TIME_UUID "f790145d-61dd-4464-9414-c058448ee9f2"

// #define CONTROL_REQUEST_UUID "420ece2e-c66c-4059-9ceb-5fc19251e453"
// #define CONTROL_RESPONSE_UUID "a9bf2905-ee69-4baa-8960-4358a9e3a558"

// #define WORK_TIME_UUID "5e9f22d4-a305-4113-8fa2-55c5d497c3f2"

// Класс для обработки событий BLE
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("Устройство ВКЛючено");
        digitalWrite(LEDq, HIGH);
    }

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("Устройство ОТКЛючено");
        digitalWrite(LEDq, LOW);

        pServer->startAdvertising(); // Перезапуск рекламы для повторного подключения
    }
};

void setup()
{
    Serial.begin(115200);
    pinMode(LEDq, OUTPUT);

    // Настройка BLE
    BLEDevice::init("ESP32-SHT30-BLE");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Создание BLE-сервиса
    BLEService *pService = pServer->createService(SERVICE_UUID);


    // Запуск сервиса
    pService->start();

    // Начало рекламы устройства
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Параметры для улучшения совместимости
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE-сервер запущен. Ожидание подключений...");
}

void loop()
{
    // Обработка подключения/отключения
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500); // Даем время для стабилизации соединения
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected)
    {
        oldDeviceConnected = deviceConnected;
    }

    // Обновление по таймеру
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        if (!isnan(t) && !isnan(h))
        {
            Serial.printf("Temp: %.2f°C, Hum: %.2f%%\n", "g", "g");

            // Отправка по BLE
            if (deviceConnected)
            {
               
                dtostrf("g", 5, 2, tempStr);
                dtostrf("g" 5, 2, humStr);
            }
            else
            {
                Serial.println("Нет подключения к устройству");
            }
        }
        else
        {
            Serial.println("Ошибка чтения данных с датчика");
        }
    }
}