#!/usr/bin/env python3
#%% 
import math
import cv2 as cv
import numpy as np
from PIL import Image
from IPython.display import display

# 상수 정의
SPEED_OF_SOUND = 343.0         # m/s
TICK_DURATION = 1.0 / 100000   # 1 tick = 1/100000 초

# 센서의 2D 위치 (m)
# 센서 순서는 파일의 데이터 순서와 일치한다고 가정 (0~5)
SENSORS = [
    (-0.5,  0.5),  # 센서 0
    ( 0.0,  0.5),  # 센서 1
    ( 0.5,  0.5),  # 센서 2
    (-0.5, -0.5),  # 센서 3
    ( 0.0, -0.5),  # 센서 4
    ( 0.5, -0.5)   # 센서 5
]

def read_datalog_file(filename):
    """ datalog.txt 파일을 읽어 숫자 튜플 리스트로 변환 (괄호 제거) """
    with open(filename, "r") as file:
        lines = file.readlines()
    data = [tuple(map(int, line.strip().replace("(", "").replace(")", "").split(','))) for line in lines]
    return data

def circle_intersections(P0, r0, P1, r1):
    """
    두 원 (중심 P0, 반지름 r0)와 (중심 P1, 반지름 r1)의 교점 계산.
    P0, P1는 (x, y) 튜플.
    반환: 교점 리스트 (최대 2개). 교점이 없으면 빈 리스트 반환.
    """
    x0, y0 = P0
    x1, y1 = P1
    dx = x1 - x0
    dy = y1 - y0
    d = math.hypot(dx, dy)
    
    # 두 원이 만나지 않거나 한 원이 다른 원을 포함하는 경우
    if d > r0 + r1 or d < abs(r0 - r1) or d == 0:
        return []
    
    a = (r0**2 - r1**2 + d**2) / (2*d)
    h = math.sqrt(max(r0**2 - a**2, 0))
    xm = x0 + a * dx / d
    ym = y0 + a * dy / d
    rx = -h * dy / d
    ry = h * dx / d
    
    intersection1 = (xm + rx, ym + ry)
    intersection2 = (xm - rx, ym - ry)
    return [intersection1, intersection2]

def estimate_position_triplet(times, indices):
    """
    주어진 센서 그룹(indices, 예: (0,1,2))에 대해,
    해당 센서의 도착 시각 (times의 해당 값)을 사용하여,
    원의 교점 방식으로 위치를 추정합니다.
    
    그룹의 첫 번째 센서를 기준 센서로 사용합니다.
    """
    i, j, k = indices
    t_i, t_j, t_k = times[i], times[j], times[k]
    Δ_j = (t_j - t_i) * TICK_DURATION * SPEED_OF_SOUND
    Δ_k = (t_k - t_i) * TICK_DURATION * SPEED_OF_SOUND
    
    P_i = SENSORS[i]
    P_j = SENSORS[j]
    P_k = SENSORS[k]
    
    best_error = float('inf')
    best_candidate = None
    best_d_i = None
    
    # d_i 후보 (센서 i에서의 거리) 0.1 m ~ 10 m 탐색 (grid search)
    for d_i in np.linspace(0.1, 10, 100):
        r_i = d_i
        r_j = d_i + Δ_j
        r_k = d_i + Δ_k
        # 센서 i와 센서 j의 원의 교점 후보 계산
        candidates = circle_intersections(P_i, r_i, P_j, r_j)
        if not candidates:
            continue
        for candidate in candidates:
            # 센서 k와의 오차: |distance(candidate, P_k) - r_k|
            d_candidate = math.hypot(candidate[0] - P_k[0], candidate[1] - P_k[1])
            error = abs(d_candidate - r_k)
            if error < best_error:
                best_error = error
                best_candidate = candidate
                best_d_i = d_i
    if best_candidate is None:
        return (0.0, 0.0)
    return best_candidate

def estimate_position_by_triplets(times):
    """
    센서 그룹을 여러 개 정의하여, 각 그룹별로 위치를 추정하고,
    최종적으로 다수결(여기서는 단순 평균)을 사용해 최종 위치를 결정합니다.
    
    사용 그룹:
      Group1: (0,1,2)
      Group2: (3,4,5)
      Group3: (0,1,3)
      Group4: (2,4,5)
    """
    groups = [(0,1,2), (3,4,5), (0,1,3), (2,4,5)]
    candidates = []
    for g in groups:
        pos = estimate_position_triplet(times, g)
        candidates.append(pos)
    # 단순 평균으로 최종 위치 산출
    avg_x = sum(pos[0] for pos in candidates) / len(candidates)
    avg_y = sum(pos[1] for pos in candidates) / len(candidates)
    return (avg_x, avg_y)

#%% 메인 실행
def main():
    filename = "datalog.txt"  # datalog.txt 파일 경로
    testData = read_datalog_file(filename)
    
    estimated_positions = []
    print("3개 센서 그룹별 원의 교점 방식(다수결) TDOA 기반 충격파 위치 추정 결과:")
    for i, data in enumerate(testData):
        # data는 8개 값 중 앞 6개가 유효하다고 가정합니다.
        pos = estimate_position_by_triplets(data[:6])
        print(f"샘플 {i}: 추정 위치 = x: {pos[0]:.4f} m, y: {pos[1]:.4f} m")
        estimated_positions.append(pos)
    
    # 시각화 (OpenCV, PIL 사용)
    width, height = 600, 600
    image = np.full((height, width, 3), 255, dtype=np.uint8)
    # 1m당 1000 픽셀 (scale = 1000)로 확대
    scale = 1000
    center_x, center_y = width // 2, height // 2
    def to_pixel(pos):
        x, y = pos
        px = int(center_x + x * scale)
        py = int(center_y - y * scale)
        return (px, py)
    
    # 센서 위치 표시 (파란 원)
    for i, sensor in enumerate(SENSORS):
        pt = to_pixel(sensor)
        cv.circle(image, pt, 5, (255, 0, 0), -1)
        cv.putText(image, f"S{i}", (pt[0]-15, pt[1]-10),
                   cv.FONT_HERSHEY_SIMPLEX, 0.5, (255,0,0), 1, cv.LINE_AA)
    
    # 각 테스트 데이터 추정 위치 (빨간 원 및 인덱스)
    for idx, pos in enumerate(estimated_positions):
        pt = to_pixel(pos)
        cv.circle(image, pt, 5, (0,0,255), -1)
        cv.putText(image, f"{idx}", (pt[0]+5, pt[1]-5),
                   cv.FONT_HERSHEY_SIMPLEX, 0.5, (0,0,255), 1, cv.LINE_AA)
    
    # 좌표축 그리기
    cv.line(image, (0, center_y), (width, center_y), (0,0,0), 1)
    cv.line(image, (center_x, 0), (center_x, height), (0,0,0), 1)
    
    image_rgb = cv.cvtColor(image, cv.COLOR_BGR2RGB)
    pil_image = Image.fromarray(image_rgb)
    display(pil_image)

if __name__ == "__main__":
    main()
#%%