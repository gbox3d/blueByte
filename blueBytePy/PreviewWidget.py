from PySide6.QtWidgets import QWidget
from PySide6.QtGui import QPainter, QPen, QBrush, QColor
from PySide6.QtCore import Qt, QRectF

class PreviewWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        # 이전에 추정된 위치들을 저장하는 리스트 (각 위치는 (x,y) 튜플, 가상 좌표 [0,1] 범위)
        self.estimated_positions = []  
        # 센서 위치 (가상의 좌표)
        self.sensor_positions = [
            (0.0, 0.0),  # sensor0
            (1.0, 0.0),  # sensor1
            (0.0, 1.0),  # sensor2
            (1.0, 1.0)   # sensor3
        ]
        self.margin = 20  # 그리기 영역 여백

    def add_estimated_position(self, pos):
        """ 새로운 추정 위치를 리스트에 추가하고 갱신 """
        self.estimated_positions.append(pos)
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # 배경 흰색 채우기
        painter.fillRect(self.rect(), QColor(255, 255, 255))

        # 그리기 영역 (여백 적용)
        rect = self.rect().adjusted(self.margin, self.margin, -self.margin, -self.margin)

        # 영역 테두리 그리기
        pen = QPen(Qt.black, 2)
        painter.setPen(pen)
        painter.drawRect(rect)

        # def map_point(p):
        #     """ 가상의 좌표 (0~1)을 rect 내 실제 좌표로 변환 (반전 없이) """
        #     x = rect.left() + p[0] * rect.width()
        #     y = rect.top() + p[1] * rect.height()  # 상단 기준, y는 그대로 증가
        #     return x, y
        
        def map_point(p):
            """ 가상의 좌표 (0~1)을 rect 내 실제 좌표로 변환 """
            x = rect.left() + p[0] * rect.width()
            # y 좌표는 Qt에서 위쪽이 0이므로 반전
            y = rect.top() + (1 - p[1]) * rect.height()
            return x, y

        sensor_radius = 5  # 센서 점의 반지름
        
        # 센서 위치 (파란 점) 그리기
        painter.setBrush(QBrush(Qt.blue))
        for sp in self.sensor_positions:
            x, y = map_point(sp)
            painter.drawEllipse(QRectF(x - sensor_radius, y - sensor_radius,
                                       sensor_radius * 2, sensor_radius * 2))

        # 이전 추정된 음원 위치들 (빨간 점) 모두 그리기
        painter.setBrush(QBrush(Qt.red))
        for pos in self.estimated_positions:
            x, y = map_point(pos)
            painter.drawEllipse(QRectF(x - sensor_radius, y - sensor_radius,
                                       sensor_radius * 2, sensor_radius * 2))

        painter.end()
        
    def clear(self):
        """ 추정 위치들을 모두 지우고 화면 갱신 """
        self.estimated_positions.clear()
        self.update()
