#ifndef DATACAPTURE_H
#define DATACAPTURE_H

#include <Arduino.h>

namespace dataCapture {

// 사용할 채널 수 (예제에서는 4개)
// constexpr int MAX_CHANNELS = 8;
#define MAX_CHANNELS 8

// 각 채널별 도착 시간을 저장 (마이크로초 단위)
// extern volatile unsigned long times[MAX_CHANNELS];

// 각 채널별 신호 수신 플래그
// extern volatile bool flags[MAX_CHANNELS];


extern int channels_num;
extern void setup(const int* pins, int num_channels);
extern boolean checkallTriggered();
extern void reset();

extern boolean g_bIsTriggered;
extern uint32_t g_ResultTicks[MAX_CHANNELS];


} // namespace dataCapture

#endif  // DATACAPTURE_H
