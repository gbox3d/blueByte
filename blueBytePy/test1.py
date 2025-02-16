#%%
import math
import cv2 as cv
import numpy as np
from PIL import Image
from IPython.display import display

# TDOA 추정 함수 (ls.py 모듈)
from solver.ls import estimate_position_tdoa

def read_datalog_file(filename):
    """ datalog.txt 파일을 읽어 숫자 튜플 리스트로 변환 (괄호 제거) """
    with open(filename, "r") as file:
        lines = file.readlines()
    data = []
    for line in lines:
        line = line.strip().replace("(", "").replace(")", "")
        if line:
            try:
                tup = tuple(map(int, line.split(',')))
                data.append(tup)
            except Exception as e:
                print(f"Error processing line: {line}\n{e}")
    return data



#%%
def drawData(img,img_size,color,_logdata):

    # 센서 위치 (가상의 좌표: (0,0), (1,0), (0,1), (1,1))를 OpenCV 좌표로 표시
    # OpenCV 좌표계: (0,0)은 왼쪽 상단, (1,1)은 오른쪽 하단
    sensor_positions = [
        (0.0, 0.0),  # sensor0 -> top-left
        (1.0, 0.0),  # sensor1 -> top-right
        (0.0, 1.0),  # sensor2 -> bottom-left
        (1.0, 1.0)   # sensor3 -> bottom-right
    ]
    for sp in sensor_positions:
        sx = int(sp[0] * (img_size - 1))
        sy = int(sp[1] * (img_size - 1))
        # 파란색 원 (반지름 4)로 센서 위치 표시
        cv.circle(img, (sx, sy), 4, (255, 0, 0), -1)

    # 로그 데이터로부터 추정 위치 계산 후 color 점으로 표시
    for i, t_us in enumerate(_logdata):
        x, y = estimate_position_tdoa(t_us)
        # x, y는 0~1 범위라고 가정. 이를 픽셀 좌표로 변환.
        px = int(x * (img_size - 1))
        py = int(y * (img_size - 1))
        
        print(f"#{i}: (x, y) = ({x:.2f}, {y:.2f}) -> ({px}, {py})")
        # 표시
        cv.circle(img, (px, py), 3, color, -1)
        

#%%
# 256x256 크기의 검은색 배경 이미지 생성
img_size = 512
img = np.zeros((img_size, img_size, 3), dtype=np.uint8)

#%%
# 로그 데이터 파일 읽기
_logdata = read_datalog_file(r"./logdata/datalog_0_0.txt")
print("로그 데이터:", _logdata)
drawData(img,img_size,(0,255,0),_logdata)
_logdata = read_datalog_file(r"./logdata/datalog_0_1.txt")
print("로그 데이터:", _logdata)
drawData(img,img_size,(0,0,255),_logdata)
_logdata = read_datalog_file(r"./logdata/datalog_0_2.txt")
print("로그 데이터:", _logdata)
drawData(img,img_size,(255,0,0),_logdata)


#%%
# OpenCV BGR 이미지를 RGB로 변환 후 PIL 이미지로 변환하여 화면에 출력
img_rgb = cv.cvtColor(img, cv.COLOR_BGR2RGB)
pil_img = Image.fromarray(img_rgb)
display(pil_img)
#%%