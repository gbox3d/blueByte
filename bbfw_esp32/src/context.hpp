#ifndef CONTEXT_HPP
#define CONTEXT_HPP


constexpr uint8_t g_version[3] = {1, 0, 0};

// // 센서 핀 번호 배열
// constexpr int sensor_PINS[] = {18,19,23,25,26,27};

// // 센서 개수 자동 계산
// constexpr int MAX = sizeof(sensor_PINS) / sizeof(sensor_PINS[0]);

// // 샘플링 속도 및 타임아웃 제한
// constexpr int sample_rate = 25000;  // 100kHz
// constexpr int timeoutlimit = 1000;    // 1000ms


void startBlink();
void stopBlink();

#endif // CONTEXT_HPP
