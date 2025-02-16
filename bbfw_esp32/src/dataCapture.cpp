#include "dataCapture.hpp"

namespace dataCapture {

// 최대 채널 수를 나타내는 매크로 (예: dataCapture.hpp에서 정의되어 있다고 가정)
// #ifndef MAX_CHANNELS
//   #define MAX_CHANNELS 8
// #endif

// 각 채널별 측정 시간을 저장하는 배열 (마이크로초 단위)
volatile unsigned long times[MAX_CHANNELS] = {0};
// 각 채널의 신호 수신 여부 플래그
volatile bool flags[MAX_CHANNELS] = {false};

int channels_num = 0;
// uint32_t g_detect_delay; // 250ms

// 인터럽트 서비스 루틴(ISR)을 생성하기 위한 매크로 (채널 번호를 인자로 사용)
#define DEFINE_ISR(channel)                   \
    void IRAM_ATTR isr_##channel() {          \
        if (!flags[channel]) {                \
            times[channel] = micros();        \
            flags[channel] = true;            \
        }                                     \
    }

// 각 채널별 ISR 정의
DEFINE_ISR(0)
DEFINE_ISR(1)
DEFINE_ISR(2)
DEFINE_ISR(3)
DEFINE_ISR(4)
DEFINE_ISR(5)
DEFINE_ISR(6)
DEFINE_ISR(7)


// ISR 함수 포인터 타입 정의
typedef void (*ISRFunc)();
// 각 채널에 해당하는 ISR 함수 포인터 배열
static ISRFunc isr_funcs[MAX_CHANNELS] = { isr_0, isr_1, isr_2, isr_3, isr_4, isr_5, isr_6, isr_7 };

uint32_t g_ResultTicks[MAX_CHANNELS];
boolean g_bIsTriggered = false;

/**
 * @brief 지정한 핀 배열에 대해 내부 풀다운 입력 및 인터럽트를 설정합니다.
 * 
 * @param pins         각 채널에 해당하는 핀 번호 배열
 * @param num_channels 채널 수 (MAX_CHANNELS와 같아야 함)
 */
void setup(const int* pins, int num_channels) {
    channels_num = num_channels;

    g_bIsTriggered = false;
    // g_detect_delay = detect_delay;
    
    for(int i = 0; i < MAX_CHANNELS; i++) {
        g_ResultTicks[i] = -1;
    }

    // 각 핀을 내부 풀다운(INPUT_PULLDOWN)으로 설정하고, 상승 에지(RISING)에서 인터럽트 발생하도록 attachInterrupt() 호출
    for (int i = 0; i < channels_num; i++) {
        pinMode(pins[i], INPUT_PULLDOWN);
        attachInterrupt(digitalPinToInterrupt(pins[i]), isr_funcs[i], RISING);
    }
}

void reset() {
    for (int i = 0; i < channels_num; i++) {
        flags[i] = false;
        times[i] = 0;
    }
    g_bIsTriggered = false;
}

/**
 * @brief 모든 채널에 신호가 수신되었을 경우, 가장 빠른 도착 시간을 기준으로 각 채널의 시차를 출력합니다.
 * 
 * @return true  모든 채널이 트리거되었으면 true를 반환
 * @return false 그렇지 않으면 false를 반환
 */
boolean checkallTriggered() {
    // 모든 채널의 플래그가 true이면 신호가 모두 수신된 것으로 간주
    bool allTriggered = true;
    for (int i = 0; i < channels_num; i++) {
        if (!flags[i]) {
            allTriggered = false;
            break;
        }
        else {
            if(micros() - times[i] > 500000) { // 500ms 이상 경과하면 false
                // allTriggered = false;
                // break;
                times[i] = 0;
                flags[i] = false;
                Serial.printf("Channel %d timeout\n", i);
            }
        }
    }
    
    if (allTriggered) {
        // 가장 빠른 도착 시간을 찾음
        unsigned long earliest = times[0];
        for (int i = 1; i < channels_num; i++) {
            if (times[i] < earliest) {
                earliest = times[i];
            }
        }

        for(int i = 0; i < channels_num; i++) {
            g_ResultTicks[i] = (u_int32_t)(times[i] - earliest);
        }

        g_bIsTriggered = true;
        
        // for (int i = 0; i < channels_num; i++) {
        //     flags[i] = false;
        //     times[i] = 0;
        // }

    }

    return allTriggered;
}



} // namespace dataCapture
