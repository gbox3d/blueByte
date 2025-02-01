#include <Arduino.h>
// #include <TaskScheduler.h>
//  #include <tonkey.hpp>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// #include <vector>

#include "etc.hpp"
// #include "app.hpp"

#include "context.hpp"

#include "packet.hpp"

//------------------------------------------------ ble start
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

// UUID for service and characteristic
#define SERVICE_UUID "30f3eb7b-1d42-422f-9e40-4ac00754ab3d"
#define CHARACTERISTIC_UUID "8ae845a8-019d-4509-b05d-938694f346d3"

class MyCharateristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    Serial.println("Characteristic write event");

    {
      // binary mode

      std::string value = pCharacteristic->getValue();

      if (value.length() >= sizeof(S_Ble_Header_Packet))
      {
        S_Ble_Header_Packet *packet = (S_Ble_Header_Packet *)value.data();

        if (packet->checkCode == CHECK_CODE)
        {
          switch (packet->cmd)
          {
          case 0x01: // about
          {
            S_Ble_Packet_About resPacket;
            resPacket.header.checkCode = CHECK_CODE;
            resPacket.header.cmd = 0x01;
            resPacket.chipId = ESP.getEfuseMac();
            resPacket.version[0] = g_version[0];
            resPacket.version[1] = g_version[1];
            resPacket.version[2] = g_version[2];
            resPacket.chennelNum = NUM_CHANNELS;
            resPacket.sampleRate = sample_rate;

            Serial.println("Res About command");
            pCharacteristic->setValue((uint8_t *)&resPacket, sizeof(resPacket));
          }
          break;

          default:
            Serial.println("Unknown command");
            break;
          }
        }
        else
        {
          Serial.println("Invalid check code");
        }
      }
      else
      {
        Serial.println("Received value too short");
      }
    }
  }
};

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    // task_Blink.disable();
    deviceConnected = true;

    Serial.println("Client connected");
    pServer->getAdvertising()->stop(); // 클라이언트가 연결되면 광고 중지
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;

    Serial.println("Client disconnected");
    pServer->getAdvertising()->start(); // 클라이언트가 연결 해제되면 광고 다시 시작
  }

  // void onMtuChanged(BLEServer* pServer,uint16_t mtu) {
  //   Serial.print("MTU size updated: ");
  //   Serial.println(mtu);
  // }
};

//------------------------------------------------ ble end
// 시차데이터 전송
boolean ble_sendTD(int* pDurationTickList, int numChannels)
{
  if (deviceConnected) // BLE 연결 확인
  {
    S_Ble_Packet_Data sendData;
    sendData.header.checkCode = CHECK_CODE;
    sendData.header.cmd = 0x09; // 0x09 명령어
    sendData.header.parm[0] = 0;
    sendData.header.parm[1] = 0;
    sendData.header.parm[2] = 0;

    // (2) 채널별 데이터 복사 (최대 8개)
    for (int ch = 0; ch < 8; ch++)
    {
      if (ch < numChannels)
        sendData.data[ch] = pDurationTickList[ch];  // 센서 데이터 채우기
      else
        sendData.data[ch] = 0; // 초과 채널은 0으로
    }

    // (3) BLECharacteristic에 값 설정 + notify
    pCharacteristic->setValue((uint8_t *)&sendData, sizeof(sendData));
    pCharacteristic->notify();

    return true;
  }
  return false;
}


void ble_setup(String strDeviceName)
{
  // // print device name and info
  /////////////////////////////////////////////////////////
  // ble setup ---------------------------------------------
  //  Create the BLE Device
  BLEDevice::init(strDeviceName.c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic->addDescriptor(new BLE2902()); // 알람 표시 기능활성화
  pCharacteristic->setCallbacks(new MyCharateristicCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  // Serial.printf("Waiting a client connection to notify...%s \n", getChipID().c_str());

  Serial.println("Ble Ready , Device name: " + strDeviceName);
}