#ifndef CONTEXT_HPP
#define CONTEXT_HPP


constexpr uint8_t g_version[3] = {1, 0, 0};

// 센서 핀 번호 배열
constexpr int sensor_PINS[] = {14, 27, 32, 33, 25, 26};

// 센서 개수 자동 계산
constexpr int NUM_CHANNELS = sizeof(sensor_PINS) / sizeof(sensor_PINS[0]);

// 샘플링 속도 및 타임아웃 제한
constexpr int sample_rate = 10000;  // 100kHz
constexpr int timeoutlimit = 1000;    // 1000ms


void startBlink();
void stopBlink();

#endif // CONTEXT_HPP
