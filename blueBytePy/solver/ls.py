# filename: ls.py
# Description: TDOA 방식으로 음원 위치 추정하는 함수 least_squares를 이용한 방법
# Author: blueByte

import math
import numpy as np
from scipy.optimize import least_squares

def tdoa_residual(vars, sensor_positions, t_s, ref_idx, c):
    """
    least_squares에서 호출되는 콜백 함수:
    (x, y) 위치에 대해, 각 센서별 '측정 거리 차' - '이론적 거리 차'의 잔차(residual)를 반환.

    - vars: [x, y]  (현재 추정 위치)
    - sensor_positions: 센서 좌표 리스트 [(x0,y0), (x1,y1), ...]
    - t_s: 센서별 도달 시간(초) 리스트
    - ref_idx: 기준 센서 인덱스 (예: 가장 먼저 받은 센서)
    - c: 음속
    """
    x, y = vars
    x_ref, y_ref = sensor_positions[ref_idx]
    d_ref = math.sqrt((x - x_ref)**2 + (y - y_ref)**2)  # 기준 센서까지의 거리
    t_ref = t_s[ref_idx]                                # 기준 센서의 도착 시간

    residuals = []
    for i, (sx, sy) in enumerate(sensor_positions):
        # 기준 센서는 어차피 (d_i - d_ref)=0이고 (t_s[i] - t_ref)=0 이므로 skip해도 됨
        if i == ref_idx:
            continue

        d_i = math.sqrt((x - sx)**2 + (y - sy)**2)
        d_theory = d_i - d_ref                     # 이론적 '거리 차'
        d_measured = c * (t_s[i] - t_ref)          # 측정된 '거리 차'
        residuals.append(d_theory - d_measured)

    return residuals

def estimate_position_tdoa(t_us,center_pos=(0.5,0.5),sensor_positions=[(0.0, 1.0),(1.0, 1.0),(0.0, 0.0),(1.0, 0.0)]):
    """
    4채널 센서의 시차(μs) 데이터를 입력받아,
    (x, y) 음원 위치를 추정하여 반환한다.

    센서 배치는:
      센서0 -> (0, 0)
      센서1 -> (1, 0)
      센서2 -> (0, 1)
      센서3 -> (1, 1)
    로 가정하고,
    t_us: 길이 4의 리스트/튜플(예: (43, 0, 137, 107)).

    Returns: (x_est, y_est)
    """

    # 1) 센서 좌표 (가상의 단위)
    sensor_positions = [
        (0.0, .5),  # sensor0: 상단 좌측
        (1.0, .5),  # sensor1: 상단 우측
        (0.0, 0.0),  # sensor2: 하단 좌측
        (1.0, 0.0)   # sensor3: 하단 우측
    ]

    # 2) 음속
    c = 340.0

    # 3) 시차(μs) -> 초(s)
    t_s = [val * 1e-6 for val in t_us]

    # 4) 기준 센서(ref_idx) 찾기
    #    보통 "가장 먼저 받은 센서" 또는 "가장 작은 t_s" 센서
    ref_idx = int(np.argmin(t_s))  # t_s 값이 가장 작은 센서 인덱스

    # 5) 초기 추정값 (센서 사각형 중앙 근처)
    x0 = center_pos[0]
    y0 = center_pos[1]
    initial_guess = np.array([x0, y0])

    # 6) least_squares로 최적화
    result = least_squares(
        tdoa_residual,           # residual 함수
        initial_guess,           # 초기값
        args=(sensor_positions, t_s, ref_idx, c),  # 추가 인자
        method='lm'              # Levenberg-Marquardt 등
        # 또는 bounds=([0,0],[1,1]) 처럼 제한할 수도 있음
    )

    x_est, y_est = result.x
    return (x_est, y_est)

if __name__ == "__main__":
    # 예시 시차 데이터 (μs)
    tdoa_data = (43, 0, 137, 107)

    x_sol, y_sol = estimate_position_tdoa(tdoa_data)
    print("추정된 음원 위치(가상 좌표):", (x_sol, y_sol))

    # 만약 실제 센서 간 거리가 10cm라면,
    #   (0,0)~(1,0)를 실제 10cm로 해석 가능
    #   => (x_sol*10, y_sol*10)이 실제 물리 좌표(cm)
    print("센서 간격 10cm라면 실제 위치(cm):", (x_sol * 10, y_sol * 10))
