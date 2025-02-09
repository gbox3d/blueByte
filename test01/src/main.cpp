#include <Arduino.h>

// 각 핀별 측정 시간을 저장할 변수 (마이크로초 단위)
volatile unsigned long _timers[4] = {0, 0, 0, 0};

// 신호가 수신되었는지 여부를 확인하는 플래그
volatile bool flags[4] = {false, false, false, false};

// 인터럽트 서비스 루틴 (ISR) - 각 핀에서 신호가 들어올 때 호출됨
void IRAM_ATTR isr01()
{
  if (!flags[0])
  {
    _timers[0] = micros();
    flags[0] = true;
  }
}

void IRAM_ATTR isr02()
{
  if (!flags[1])
  {
    _timers[1] = micros();
    flags[1] = true;
  }
}

const int pin_indice[4] = {18, 19, 26, 26};
int time_diffs[4] = {0, 0, 0, 0};

void setup()
{
  Serial.begin(115200);

  // 핀 설정
  for (int i = 0; i < 4; i++)
  {
    pinMode(pin_indice[i], INPUT_PULLDOWN);
  }

  // 인터럽트 설정 (상승 에지 감지)
  attachInterrupt(digitalPinToInterrupt(pin_indice[0]), isr01, RISING);
  attachInterrupt(digitalPinToInterrupt(pin_indice[1]), isr02, RISING);

  Serial.println("Start");
}

enum State
{
  WAIT,
  RECEIVED,
  TIME_OVER
};

void loop()
{

  // 각 핀별로 신호가 들어왔는지 확인
  //  bool is_all_received = true;
  State state = RECEIVED;

  for (int i = 0; i < 2; i++)
  {
    if (!flags[i])
    {
      // is_all_received = false;
      state = WAIT;
      break;
    }
  }

  if (state == RECEIVED)
  {
    for (int i = 0; i < 2; i++)
    {
      time_diffs[i] = _timers[i] - _timers[0];
      if (time_diffs[i] > 3000)
      {
        Serial.printf("Time Over: pin : #d , %d\n", 1, time_diffs[i]);
        state = TIME_OVER;
        break;
      }
    }
  }

  switch (state)
  {
  case WAIT:
    break;
  case RECEIVED:
  {
    Serial.printf("Time Diff: %d, %d\n", time_diffs[0], time_diffs[1]);
    
  }
  case TIME_OVER:
  {
    delay(1000);
    Serial.println("Next");
    // 다음 신호를 받기 위해 플래그 초기화
    for (int i = 0; i < 2; i++)
    {
      flags[i] = false;
    }
  }
  break;
  }

  delay(100);
}
