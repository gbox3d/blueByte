# 파일명 : mainwindow.py

import sys
import struct

from time import sleep

from PySide6.QtCore import QUuid, QByteArray
from PySide6.QtWidgets import QApplication, QWidget, QMainWindow
from PySide6.QtBluetooth import QLowEnergyController, QLowEnergyService, QBluetoothUuid, QBluetoothDeviceInfo, QLowEnergyCharacteristic


from UI.mainwindow import Ui_MainWindow
from DlgDeviceList import DlgDeviceList

# 미리 보기 위젯 클래스 (위의 코드를 포함시키거나 별도 모듈에서 임포트)
from PreviewWidget import PreviewWidget  # 만약 별도 파일로 저장했다면
# 또는 mainwindow.py 내에 직접 위의 PreviewWidget 코드를 삽입

# TDOA 추정 함수 (ls.py 모듈)
from solver.ls import estimate_position_tdoa

CHECK_CODE = 250130


class MainWindow(QMainWindow, Ui_MainWindow):
    def __init__(self):
        super().__init__()
        self.setupUi(self)
        self.actionscan.triggered.connect(self.onActionScan)
        
        self.service_uuid = QBluetoothUuid(QUuid("30f3eb7b-1d42-422f-9e40-4ac00754ab3d"))
        self.characteristic_uuid = QBluetoothUuid(QUuid("8ae845a8-019d-4509-b05d-938694f346d3"))
        
        self.characteristic = None
        self.device_info = None

        self.controller = None
        self.service = None
        
        # self.actiondisconnect.enabled = False
        self.actiondisconnect.triggered.connect(self.onActionDisconnect)
        
        # self.actionsend_about.enabled = False
        self.actionsend_about.triggered.connect(self.onActionAbout)
        
        # 미리 보기 초기화
        self.actionclear_preView.triggered.connect(self.onClearPreview)
        
        # 미리 보기 프레임 추가: centralwidget 내의 빈 공간에 배치
        # pte_Logs는 아래쪽에 위치하므로, 위쪽 영역에 미리 보기 위젯을 만듭니다.
        self.previewWidget = PreviewWidget(self.centralwidget)
        self.previewWidget.setGeometry(10, 10, 781, 380)  # 위치와 크기는 필요에 따라 조정

    def onActionScan(self):
        """ BLE 장치 검색 대화상자를 실행 """
        print("BLE 장치 검색 시작...")
        dlg = DlgDeviceList(self)
        dlg.deviceSelected.connect(self.onDeviceSelected)
        dlg.exec()
        
    def onActionDisconnect(self):
        """ BLE 연결 해제 """
        if self.controller:
            self.controller.disconnectFromDevice()
            self.controller = None
            self.service = None
            self.characteristic = None
            self.device_info = None
            print("BLE 연결 해제됨.")
        else:
            print("연결된 장치가 없습니다.")
            
    def onActionAbout(self):
        """ about 커맨드 전송 """
        self.sendAboutCommand()

    def onDeviceSelected(self,device_info : QBluetoothDeviceInfo):
        """ 선택한 장치 정보 받아서 BLE 연결 시도 """
        address = device_info.address().toString()
        print(f"선택한 장치: {device_info.name()} ({address})")
        self.connectToDevice(device_info)

    def connectToDevice(self, device_info: QBluetoothDeviceInfo):
        """ BLE 장치에 연결 """
        print(f"디바이스 {device_info.address().toString()}에 연결 시도 중...")

        self.controller = QLowEnergyController.createCentral(device_info, self)
        self.controller.connected.connect(self.onConnected)
        self.controller.disconnected.connect(self.onDisconnected)
        self.controller.errorOccurred.connect(self.onError)
        self.controller.connectToDevice()
            

    def onConnected(self):
        """ BLE 연결 성공 시 서비스 검색 """
        print("BLE 장치에 연결되었습니다.")
        self.controller.discoverServices()
        self.controller.serviceDiscovered.connect(self.onServiceDiscovered)

    def onServiceDiscovered(self, service_uuid):
        """ 특정 서비스 발견 시 처리 """
        if service_uuid == self.service_uuid:
            print(f"서비스 발견: {service_uuid}")
            self.service = self.controller.createServiceObject(service_uuid)
            if self.service:
                self.service.stateChanged.connect(self.onServiceStateChanged)
                self.service.discoverDetails()
        else:
            print(f"다른 서비스 발견: {service_uuid}")

    def enableNotification(self, chara: QLowEnergyCharacteristic):
        """ CCCD(0x2902)에 0x01,0x00을 써서 Notify 활성화 """
        ccc_descriptor = chara.descriptor(QBluetoothUuid.DescriptorType.ClientCharacteristicConfiguration)
        if ccc_descriptor.isValid():
            # Notify 활성화 (0x01,0x00). Indicate를 쓰려면 0x02,0x00
            self.service.writeDescriptor(ccc_descriptor, QByteArray(b'\x01\x00'))
            print("Notify 활성화 요청")

            # Notify 콜백 연결
            self.service.characteristicChanged.connect(self.onCharacteristicChanged)
            
    def onServiceStateChanged(self, state):
        if state == QLowEnergyService.ServiceDiscovered:
            print("서비스 세부 정보 검색 완료.")
            self.characteristic = self.service.characteristic(self.characteristic_uuid)
            if self.characteristic.isValid():
                print(f"캐릭터리스틱 발견: {self.characteristic.uuid().toString()}")
                
                self.enableNotification(self.characteristic) # Notify 활성화
                
                sleep(1)
                self.sendAboutCommand()
                # self.actionabout.enabled = True
                # self.actionconnect.enabled = True
    def onReceiveTD_Data(self, data_values): 
        print("TD Data")
        with open("datalog.txt", 'a') as f:
            f.write(str(data_values) + "\n")
        
        # TDOA 데이터가 4채널 이상이라면 처음 4채널 데이터 사용
        tdoa_data = data_values[:4]
        # 추정 위치 계산 (가상 좌표, [0,1] 범위)
        estimated_pos = estimate_position_tdoa(tdoa_data)
        self.previewWidget.add_estimated_position(estimated_pos)
        self.pte_Logs.appendPlainText(f"추정된 위치 (가상 좌표): {estimated_pos}")
        
        
    def onCharacteristicChanged(self, chara, value):
        """
        Notify/Indicate 데이터가 들어오면 호출됨.
        여기서 S_Ble_Packet_About(24바이트) 응답을 파싱한다.
        """
        if chara.uuid() == self.characteristic_uuid:
            hex_str = value.toHex().data().decode('ascii')  # "abcd1234..." 형식의 문자열
            print(f"[onCharacteristicChanged] 수신({len(value)}바이트): {hex_str}")

            # S_Ble_Header_Packet = 8 bytes
            # <I B 3B
            # S_Ble_Packet_About = 24 bytes
            
            # <I B 3B Q 3B B I
            # checkCode(4), cmd(1), parm[3], chipId(8), version[3], chennelNum(1), sampleRate(4)
            
            #S_Ble_Packet_Data = 40 bytes
            #<I B 3B 8I
            #checkCode(4), cmd(1), parm[3], data[8]
            
            # 헤더 파싱
            if(len(value) >= 8):
                
                # 8byte 카피
                _header = value[:8]
                check_code, cmd ,p1,p2,p3= struct.unpack('<I B B B B', _header)
                
                if check_code == CHECK_CODE:
                    if cmd == 0x01:
                        print("=== About 응답 수신 ===")
                        _body = value[8:]
                        if len(value) == 24:
                            chip_id, ver0, ver1, ver2, _, _ = struct.unpack('<Q B B B B I', _body)
                            print(f"chipId       : 0x{chip_id:X}")
                            print(f"version      : {ver0}.{ver1}.{ver2}")
                            # print(f"chennelNum   : {ch_num}")
                            # print(f"sampleRate   : {sample_rate}")
                            
                            self.pte_Logs.appendPlainText(f"chipId       : 0x{chip_id:X}")
                            self.pte_Logs.appendPlainText(f"version      : {ver0}.{ver1}.{ver2}")
                            # self.pte_Logs.appendPlainText(f"chennelNum   : {ch_num}")
                            # self.pte_Logs.appendPlainText(f"sampleRate   : {sample_rate}")
                        else:
                            print("잘못된 About 응답 패킷 크기")
                    elif cmd == 0x09:
                        print("=== Data 패킷 수신 (cmd=0x09) ===")
                        if len(value) == 40:
                            _body = value[8:]
                            data_values = struct.unpack('<8I', _body)
                            
                            print(f"수신된 데이터: {data_values}")
                            self.pte_Logs.appendPlainText(f"수신된 데이터: {data_values}")
                            
                            self.onReceiveTD_Data(data_values)
                        else:
                            print("잘못된 Data 응답 패킷 크기")
                        
                        
                else:
                    print("About 응답이 아닌 다른 데이터 or 잘못된 패킷.")
            
            else:
                print("패킷 길이가 올바르지 않습니다.")
                
    
    def sendAboutCommand(self):
        if self.characteristic and self.characteristic.isValid():
            print("[sendAboutCommand] 0x01, about 커맨드 전송")

            # little-endian 직렬화
            packet_bytes = struct.pack('<I B 3B', CHECK_CODE, 0x01, 0, 0, 0)

            # QByteArray로 변환
            packet = QByteArray(packet_bytes)

            # BLE 특성에 write
            self.service.writeCharacteristic(self.characteristic, packet)
            print(f"전송 바이트: {packet.toHex()}")
            
    def onClearPreview(self):
        """ 미리 보기 위젯 초기화 """
        self.previewWidget.clear()

    def onDisconnected(self):
        """ BLE 장치가 연결 해제되었을 때 처리 """
        print("BLE 장치 연결 해제됨.")
        self.controller = None
        self.service = None
        self.characteristic = None
        self.device_info = None
        
        self.actionsend_about.enabled = False
        self.actiondisconnect.enabled = False

    def onError(self, error):
        """ BLE 연결 오류 처리 """
        print(f"BLE 연결 오류 발생: {error}")
        
    
if __name__ == "__main__":
    app = QApplication(sys.argv)
    widget = MainWindow()
    widget.show()
    sys.exit(app.exec())