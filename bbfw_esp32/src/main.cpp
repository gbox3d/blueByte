/*
bluebyte 프로토콜의 레퍼런스 기기를 위한 펌웨어 입니다.

*/

#include <Arduino.h>
#include "dataProcess.hpp"

#include <TaskScheduler.h>
#include <ArduinoJson.h>

#include "config.hpp"
#include "etc.hpp"

#include "context.hpp"

#include "packet.hpp"

#if not defined(BUILTIN_LED)

#if defined(LOLIN_D32) | defined(LOLIN_D32_PRO)
// this device aready defined LED_BUILTIN 4 -> D5
#elif defined(WROVER_KIT)
#define BUILTIN_LED 5

#elif defined(SEED_XIAO_ESP32C3)
#define BUILTIN_LED D10

#endif
#endif

// global variables
Scheduler g_ts;
Config g_config;

// command parser
extern String parseCmd(String _strLine);

// BLE
extern void ble_setup(String strDeviceName);
extern bool deviceConnected;
extern boolean ble_sendTD(int *pDurationTickList, int numChannels); // 시차데이터 전송

// data process
CDataProcess dataProcess(sensor_PINS, NUM_CHANNELS, sample_rate, timeoutlimit);

Task task_Cmd(100, TASK_FOREVER, []()
              {
    if (Serial.available() > 0)
    {
        String _strLine = Serial.readStringUntil('\n');
        _strLine.trim();
        
        // Serial.println(_strLine);

        Serial.println(parseCmd(_strLine));
    } }, &g_ts, true);

Task task_LedBlink(500, TASK_FOREVER, []()
                   { digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED)); }, &g_ts, true);

void startBlink()
{
  task_LedBlink.enable();
}

void stopBlink()
{
  task_LedBlink.disable();
  if (deviceConnected)
  {
    digitalWrite(BUILTIN_LED, LOW);
    Serial.println("LED_BUILTIN : " + String(BUILTIN_LED) + " " + String(digitalRead(BUILTIN_LED)));
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH);
  }
}

//------------------------------------------------

TaskHandle_t taskHandle; // 태스크 핸들
void dataLoop(void *param)
{
  CDataProcess *instance = static_cast<CDataProcess *>(param);
  int nFSM = 0;
  u_int32_t tick = millis();

  while (true)
  {

    boolean bDetected = instance->readData();

    switch (nFSM)
    {
    case 0:
      if (bDetected)
      {
        int *pDurationTickList = instance->getDurationTickList();
        int numChannels = instance->getNumChannels();

        // 최소 tick 값을 찾기
        int minTick = pDurationTickList[0];
        for (int i = 1; i < numChannels; i++)
        {
          if (pDurationTickList[i] < minTick)
          {
            minTick = pDurationTickList[i];
          }
        }

        // 각 채널의 tick 차이를 계산하여 출력 (최소값은 0)
        for (int i = 0; i < numChannels; i++)
        {
          int diff = pDurationTickList[i] - minTick;
          Serial.print(diff);
          Serial.print(" ");
        }
        Serial.println();

        if (ble_sendTD(pDurationTickList, numChannels))
        {
          Serial.println("Send TD");
        }

        nFSM = 1;
        tick = millis();
      }
      break;
    case 1:
      // Serial.println(millis() - tick);
      if (millis() - tick > 250)
      {
        instance->reset();
        nFSM = 0;
        Serial.println("Reset");
      }

      break;

    default:
      break;
    }

    // g_ts.execute();
  }
}

TaskHandle_t taskHandle_App;
void appLoop(void *param)
{
  while (true)
  {
    g_ts.execute();
  }
}

void setup()
{
  String strDeviceName = "ESP32_BLE" + String(getChipID().c_str());

  Serial.begin(115200);

  delay(1000);
  Serial.println("Start");

  dataProcess.setup();         // 데이터 처리 클래스 설정
  dataProcess.startSampling(); // 샘플링 시작

  // 태스크 생성 (코어 1에 고정)
  xTaskCreatePinnedToCore(
      dataLoop,     // 태스크 함수
      "dataLoop",   // 태스크 이름
      4096,         // 스택 크기
      &dataProcess, // 태스크에 전달할 인수
      1,            // 우선순위
      &taskHandle,  // 태스크 핸들
      1             // 코어 1에 고정
  );
  if (taskHandle == NULL)
  {
    Serial.println("Task creation failed!");
  }
  else
  {
    Serial.println("Task created successfully.");
  }

  // app task 생성
  xTaskCreatePinnedToCore(
      appLoop,         // 태스크 함수
      "appLoop",       // 태스크 이름
      4096,            // 스택 크기
      NULL,            // 태스크에 전달할 인수
      1,               // 우선순위
      &taskHandle_App, // 태스크 핸들
      1                // 코어 1에 고정
  );

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH); // turn the LED off by making the voltage LOW

  Serial.begin(115200);

  g_config.load();

  delay(250);

  Serial.println(":-]");
  Serial.println("Serial connected");

  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Chip ID: %s\n", getChipID().c_str());

  // 보드에 대한 정보를 출력
  Serial.printf("Board: %s\n", ARDUINO_BOARD);
  Serial.println("LED_BUILTIN: " + String(BUILTIN_LED));

  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

  // BLE setup
  ble_setup(strDeviceName);

  // task manager start
  g_ts.startNow();
}

void loop()
{
}
