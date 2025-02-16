/*
bluebyte 프로토콜의 레퍼런스 기기를 위한 펌웨어 입니다.

*/

#include <Arduino.h>


#include <TaskScheduler.h>
#include <ArduinoJson.h>

#include "config.hpp"
#include "etc.hpp"

#include "context.hpp"

#include "packet.hpp"

#include "dataCapture.hpp"

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

uint32_t g_detect_delay;

// command parser
extern String parseCmd(String _strLine);

// BLE
extern void ble_setup(String strDeviceName);
extern bool deviceConnected;
extern boolean ble_sendTD(int *pDurationTickList, int numChannels); // 시차데이터 전송


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
  int _results[MAX_CHANNELS];
  for(int i = 0; i < dataCapture::channels_num; i++) {
    _results[i] = -1; // 초기화
  }
  
  while (true)
  {
    

    if(dataCapture::checkallTriggered()) {
      
      // 시차 데이터 전송
      for(int i = 0; i < dataCapture::channels_num; i++) {
        _results[i] = dataCapture::g_ResultTicks[i];
        Serial.printf("%d ", _results[i]);
      }
      for(int i = dataCapture::channels_num; i < MAX_CHANNELS; i++) {
        _results[i] = -1;
      }
      Serial.println();

      if(ble_sendTD(_results, dataCapture::channels_num)) {
        Serial.println("BLE sendTD success");
      } else {
        Serial.println("BLE sendTD failed");
      }

      // vTaskDelay(100 / portTICK_PERIOD_MS);
      vTaskDelay( g_detect_delay/ portTICK_PERIOD_MS);

      dataCapture::reset();

    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
    
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

  String strDeviceName = "BB32_" + String(getChipID().c_str());

  Serial.begin(115200);
  delay(1000);
  Serial.printf("Start : %s\n", strDeviceName.c_str());

  g_config.load();

  int channels_num = g_config.get<int>("ch_num", 2);
  g_detect_delay = g_config.get<uint32_t>("detect_delay", 250);

  Serial.printf("channels_num : %d\n", channels_num);
  Serial.printf("detect_delay : %d\n", g_detect_delay);


  int sensor_PINS[] = {18, 19, 23, 25, 26, 27};

  if(g_config.hasKey("sensorPins")) {

    Serial.println("sensorPins key exist");

    JsonDocument _doc_sensorpins;
    g_config.getArray("sensorPins", _doc_sensorpins);

    JsonArray _pins = _doc_sensorpins.as<JsonArray>();

    int _index = 0;
    for (JsonVariant v : _pins)
    {
        int pin = v.as<int>();

        sensor_PINS[_index] = pin;
        // Serial.printf("%2d sensor pin : %d\n", _index, pin);
        _index++;
    }
}
else {
  Serial.println("sensorPins key not exist");
}

for(int i = 0; i < channels_num; i++) {
  Serial.printf("%2d sensor pin : %d\n", i, sensor_PINS[i]);
}

  dataCapture::setup(sensor_PINS, channels_num);

  // 태스크 생성 (코어 1에 고정)
  xTaskCreatePinnedToCore(
      dataLoop,     // 태스크 함수
      "dataLoop",   // 태스크 이름
      4096,         // 스택 크기
      // &dataProcess, // 태스크에 전달할 인수
      NULL,
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
