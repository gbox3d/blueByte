// dataProcess.hpp
#pragma once

#include <Arduino.h>

// 큐 크기
#define QUEUE_SIZE 1024
#define MAX_CHANNELS 8

/*

100khz 는 10us 주기

1000 * 10us = 10ms

음속은 340m/s = 0.34mm/us

10us의 오차 범위는 3.4mm
10000us 오차범위는 3400mm = 3.4m

*/

class CDataProcess
{
public:
    CDataProcess(const int *pins, int num_channels, int sample_rate,int timeoutlimit=1000);
    ~CDataProcess();

    void setup();
    void startSampling();
    void stopSampling();

    boolean readData();

    bool isDetected()
    {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            if (!_DetectCheckList[i])
                return false;
        }
        return true;
    }

    bool m_bSkip = false;

    int m_Index = 0;

    // 전역 변수
    QueueHandle_t dataQueue; // FreeRTOS 큐
    

    int *getDurationTickList()
    {
        return _DetectTickList;
    }

    int getNumChannels()
    {
        return NUM_CHANNELS;
    }

    void reset() {
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            _DetectCheckList[i] = false;
            _DetectTickList[i] = 0;
        }
        m_Index = 0;
        m_bSkip = false;
    }

private:
    const int *sensor_PINS;
    int NUM_CHANNELS; // 최대 8체널
    int SAMPLE_RATE;

    int nFSM = 0;
    int _durationTick = 0;

    u_int32_t m_timeoutlimit;

    boolean _DetectCheckList[MAX_CHANNELS];
    int _DetectTickList[MAX_CHANNELS];

    hw_timer_t *timer;
    
    static CDataProcess *instance;

    static void IRAM_ATTR onTimer();
};