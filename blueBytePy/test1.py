#%%
import math
import cv2 as cv
import numpy as np
from PIL import Image
from IPython.display import display

#%%

def read_datalog_file(filename):
    """ datalog.txt 파일을 읽어 숫자 튜플 리스트로 변환 (괄호 제거) """
    with open(filename, "r") as file:
        lines = file.readlines()
    data = [tuple(map(int, line.strip().replace("(", "").replace(")", "").split(','))) for line in lines]
    return data

absoluteTicks = read_datalog_file("datalog.txt")

#%%
# 6개의 센서 값에서 가장 작은 값을 찾아서 그것을 기준으로 차를 구한다.
def findMinValue(_datas):
    minVal = _datas[0]
    for i in range(1, 6):
        if minVal > _datas[i]:
            minVal = _datas[i]
            
    _result = []
    for i in range(6):
        _result.append(_datas[i] - minVal)
    
    return _result
#%%
 
_result = findMinValue(absoluteTicks[1])
print(_result)


# %%
