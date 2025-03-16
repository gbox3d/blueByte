#include <Arduino.h>
#include <avr/io.h>

// 설정 변수
const int threshold = 45;       // 임계값 설정
const int pulsePin = 2;         // 펄스 출력용 디지털 핀
const int pulseDuration = 50;   // 펄스 출력 지속 시간 (밀리초)

void setup() {
  Serial.begin(115200);
  // 펄스 출력 핀 설정 (직접 포트 조작으로 대체 가능)
  pinMode(pulsePin, OUTPUT);
  // pinMode(13, OUTPUT);
  
  // ADC 설정 - 8비트 고속 모드
  ADMUX = (1 << REFS0)   // AVcc를 기준 전압으로 선택
        | (1 << ADLAR);  // 왼쪽 정렬 - 8비트 해상도 사용
  
  ADCSRA = (1 << ADEN)   // ADC 활성화
        | (1 << ADPS0);  // 프리스케일러를 2로 설정 (16MHz/2 = 8MHz ADC 클록)
  
  // 첫 번째 변환을 수행하여 ADC를 초기화
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  Serial.println("Ready mic393 v1.0 rev 2");


}

void loop() {
  static int8_t prev = ADCH; // 첫 번째 읽은 값으로 초기화
  
  // 고속 샘플링 (폴링 방식)
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));
  
  // 8비트 해상도로 값 읽기
  int8_t current = ADCH;
  
  // 이전 값과의 차이 계산 및 절대값
  int8_t diff = current - prev;
  if (diff < 0) diff = -diff;
  
  // 임계값 이상의 변화가 감지되면 펄스 발생
  if (diff >= threshold) {
    // 직접 포트 조작으로 더 빠른 GPIO 출력
    PORTD |= (1 << pulsePin);  // HIGH 설정 (Arduino UNO 기준, 핀 2는 PORTD의 비트 2)
    // delayMicroseconds(pulseDuration);
    delay(pulseDuration);
    PORTD &= ~(1 << pulsePin); // LOW 설정

    digitalWrite(13, HIGH);
    delay(250);
    digitalWrite(13, LOW);
    
    // 변화 감지 후 새 값으로 업데이트
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    current = ADCH;
  }
  
  prev = current;  // 현재 값을 이전 값으로 업데이트
}