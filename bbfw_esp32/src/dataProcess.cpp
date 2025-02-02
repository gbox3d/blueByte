#include "dataProcess.hpp"

CDataProcess *CDataProcess::instance = nullptr;

CDataProcess::CDataProcess(const int *pins, int num_channels, int sample_rate, int timeoutlimit)
    : sensor_PINS(pins), NUM_CHANNELS(num_channels), SAMPLE_RATE(sample_rate), timer(nullptr)
{
    instance = this;
    m_timeoutlimit = timeoutlimit; // 예: 100khz 10us 주기, 1000번 = 10ms, 3.4m 이내에 감지되지 않으면 타임아웃

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        _DetectCheckList[i] = false;
        _DetectTickList[i] = 0;
    }
}

CDataProcess::~CDataProcess()
{
    stopSampling();
}

void CDataProcess::setup()
{
    // 데이터 핀 설정
    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        pinMode(sensor_PINS[i], INPUT);
    }

    // FreeRTOS 큐 생성
    dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(uint8_t)); // 1바이트씩 저장

    // 타이머 설정 (1us 단위)
    timer = timerBegin(0, 80, true);                           // 타이머 0, 80분주, 카운트업 모드
    timerAttachInterrupt(timer, &CDataProcess::onTimer, true); // ISR 핸들러 연결
    timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);       // 샘플링 주기 설정
}

void CDataProcess::startSampling()
{
    if (timer)
    {
        timerAlarmEnable(timer); // 타이머 시작
    }
}

void CDataProcess::stopSampling()
{
    if (timer)
    {
        timerAlarmDisable(timer); // 타이머 정지
    }
}

void IRAM_ATTR CDataProcess::onTimer()
{
    if (!instance)
        return;

    // 타이머 인터럽트 시점에 인덱스 증가
    instance->m_Index++;

    uint8_t sampled_data = 0;
    for (int i = 0; i < instance->NUM_CHANNELS; i++)
    {
        sampled_data |= ((GPIO.in >> instance->sensor_PINS[i]) & 0x1) << i;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(instance->dataQueue, &sampled_data, &xHigherPriorityTaskWoken);

    // 태스크 컨텍스트 전환이 필요하면 전환
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

boolean CDataProcess::readData()
{
    uint8_t data;

    if (xQueueReceive(dataQueue, &data, portMAX_DELAY) == pdPASS)
    {
        // 타이머 ISR에서 이미 m_Index가 증가되므로 readData에서는 인덱스 증가는 하지 않음.
        for (int ic = 0; ic < NUM_CHANNELS; ic++)
        {
            if ((data & (1 << ic)) && !_DetectCheckList[ic])
            {
                _DetectCheckList[ic] = true;
                _DetectTickList[ic] = m_Index;  // 현재 m_Index 값을 저장
            }
        }

        // 타임아웃 체크
        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            int _timeout = m_Index - _DetectTickList[i];
            if (_timeout > m_timeoutlimit)
            {
                _DetectCheckList[i] = false;
                _DetectTickList[i] = 0;
            }
        }
    }
    
    if (isDetected() && !m_bSkip)
    {
        m_bSkip = true;
        return true;
    }
    
    return false;
}
