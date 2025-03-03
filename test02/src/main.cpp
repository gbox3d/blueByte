#include <Arduino.h>
#include <avr/io.h>

// 설정 변수
boolean useHighResolution = false;  // false: 8비트, true: 10비트
int threshold = 45;

// 샘플링 속도 측정을 위한 변수
unsigned long intervalStartTime = 0;
unsigned long intervalSampleCount = 0;
unsigned long lastRateCheck = 0;
const unsigned long RATE_CHECK_INTERVAL = 5000; // 5초마다 샘플링 속도 체크

void setup() {
  Serial.begin(115200);  // 시리얼 시작 (115200 baud)
  
  // ADC 설정 - 사용자 선택에 따른 해상도 설정
  if (useHighResolution) {
    // 10비트 해상도 설정
    ADMUX = (1 << REFS0);  // AVcc를 기준 전압으로 선택, 오른쪽 정렬(기본값)
    ADCSRA = (1 << ADEN)   // ADC 활성화
          | (1 << ADPS1) | (1 << ADPS0);  // 프리스케일러를 8로 설정 (16MHz/8 = 2MHz ADC 클록)
    Serial.println("시작: 아날로그 고속 샘플링 (10비트 해상도 모드)");
  } else {
    // 8비트 해상도 설정
    ADMUX = (1 << REFS0)   // AVcc를 기준 전압으로 선택
          | (1 << ADLAR);  // 왼쪽 정렬 - 8비트 해상도 사용
    ADCSRA = (1 << ADEN)   // ADC 활성화
          | (1 << ADPS0);  // 프리스케일러를 2로 설정 (16MHz/2 = 8MHz ADC 클록)
    Serial.println("시작: 아날로그 고속 샘플링 (8비트 해상도, 고속 모드)");
  }
  
  // 첫 번째 변환을 수행하여 ADC를 초기화
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC)); // 첫 번째 변환은 기다림
  
  intervalStartTime = millis();
  lastRateCheck = millis();
}

void loop() {
  static int16_t prev = -1; // 초기값을 -1로 설정하여 첫 실행 식별
  unsigned long currentTime = millis();
  int16_t current;
  
  // 고속 샘플링 (폴링 방식)
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC)); // 변환 완료 대기
  
  // 해상도 설정에 따라 적절한 레지스터에서 값 읽기
  if (useHighResolution) {
    current = ADC;   // 10비트 값 읽기 (0-1023)
  } else {
    current = ADCH;  // 8비트 값 읽기 (0-255)
  }
  
  // 샘플 카운트 증가
  intervalSampleCount++;
  
  // 초기 이전 값 설정 (첫 실행 시)
  if (prev == -1) {
    prev = current;
    return; // 첫 번째 루프에서는 여기서 종료
  }
  
  // 이전 값과의 차이 계산 (부호 있는 정수로 계산 후 절대값 취함)
  int16_t diff = current - prev;
  if (diff < 0) diff = -diff;  // 절대값 계산
  
  if (diff >= threshold) {
    unsigned long t = micros();
    Serial.print("변화 감지! 시간(μs): ");
    Serial.println(t);
    Serial.print("현재 값: ");
    Serial.println(current);
    Serial.print("이전 값: ");
    Serial.println(prev);
    Serial.print("차이: ");
    Serial.println(diff);
    Serial.println("reset sensing");
    delay(500);

    // 고속 샘플링 (폴링 방식)
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)); // 변환 완료 대기
    
    // 해상도 설정에 따라 적절한 레지스터에서 값 읽기
    if (useHighResolution) {
      current = ADC;   // 10비트 값 읽기 (0-1023)
    } else {
      current = ADCH;  // 8비트 값 읽기 (0-255)
    }
  }
  
  // 5초마다 샘플링 속도 체크 및 표시
  if (currentTime - lastRateCheck >= RATE_CHECK_INTERVAL) {
    // 현재 간격의 샘플링 속도 계산 (초당 샘플 수)
    float intervalSeconds = (currentTime - intervalStartTime) / 1000.0;
    float samplingRate = intervalSampleCount / intervalSeconds;
    
    // 정적 변수로 총계를 유지
    static unsigned long totalSamples = 0;
    static unsigned long totalSeconds = 0;
    
    totalSamples += intervalSampleCount;
    totalSeconds += RATE_CHECK_INTERVAL / 1000; // 5초를 더함
    
    Serial.print("현재 ADC 값: ");
    Serial.println(current);
    Serial.print("현재 해상도: ");
    Serial.println(useHighResolution ? "10비트 (0-1023)" : "8비트 (0-255)");
    Serial.print("현재 샘플링 속도: ");
    Serial.print(samplingRate, 2);
    Serial.println(" 샘플/초");
    Serial.print("이 간격의 샘플 수: ");
    Serial.println(intervalSampleCount);
    Serial.print("총 샘플 수: ");
    Serial.println(totalSamples);
    Serial.print("총 경과 시간: ");
    Serial.print(totalSeconds);
    Serial.println(" 초");
    
    // 1초 딜레이 후 재시작
    delay(1000);
    Serial.println("샘플링 재시작...");
    
    // 다음 간격을 위한 변수 초기화
    lastRateCheck = millis();
    intervalStartTime = millis();
    intervalSampleCount = 0;
  }
  
  prev = current;  // 현재 값을 이전 값으로 업데이트
}

// 시리얼로 명령을 받아 해상도를 변경하는 기능
void serialEvent() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '8') {
      useHighResolution = false;
      Serial.println("8비트 고속 모드로 전환합니다...");
      setup(); // ADC 설정 다시 적용
    } else if (cmd == '1') {
      useHighResolution = true;
      Serial.println("10비트 모드로 전환합니다...");
      setup(); // ADC 설정 다시 적용
    }
  }
}