# 파일명 : DlgDeviceList.py

from PySide6 import QtWidgets
from PySide6.QtCore import QFile, QThread, Signal, Qt
from PySide6.QtGui import QStandardItemModel, QStandardItem

import time
from UI.DlgDeviceList import Ui_DialogDeviceList


from PySide6.QtBluetooth import (
    QBluetoothDeviceDiscoveryAgent, QBluetoothDeviceInfo
)
        
class DlgDeviceList(QtWidgets.QDialog, Ui_DialogDeviceList):
    deviceSelected = Signal(QBluetoothDeviceInfo)  # (Name, Address)
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.setupUi(self)
        
        self.model = QStandardItemModel(self)
        self.model.setHorizontalHeaderLabels(["Name", "Address", "RSSI"])
        self.tableViewDeviceList.setModel(self.model)
        
        header = self.tableViewDeviceList.horizontalHeader()
        # header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        # header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        # header.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        
        header.resizeSection(0, int(self.tableViewDeviceList.width() * 0.5))
        header.resizeSection(1, int(self.tableViewDeviceList.width() * 0.25))
        header.resizeSection(2, int(self.tableViewDeviceList.width() * 0.25))
        
        
        self.device_dict = {}  # chipid를 키로 사용하여 중복 제거
        self.device_info_map = {}  # 주소 기반으로 `QBluetoothDeviceInfo` 저장
        
        # 별도의 QThread 대신, 메인 스레드에서 사용
        self.agent = QBluetoothDeviceDiscoveryAgent()
        self.agent.deviceDiscovered.connect(self.onDeviceDiscovered)
        self.agent.finished.connect(self.onScanFinished)

        # BLE 스캔 시작 (LowEnergyMethod 명시)
        self.agent.setLowEnergyDiscoveryTimeout(10000)  # 10초
        self.agent.start(QBluetoothDeviceDiscoveryAgent.LowEnergyMethod)
        
    def onDeviceDiscovered(self, device: QBluetoothDeviceInfo):
        name = device.name() or "Unknown"
        address = device.address().toString()
        rssi = device.rssi()
        print(name, address, rssi)
        
        # 중복 제거
        chipid = address
        if chipid in self.device_dict:
            return
        self.device_dict[chipid] = True
        self.device_info_map[address] = device
        
        # 테이블에 추가
        row = self.model.rowCount()
        self.model.insertRow(row)
        self.model.setData(self.model.index(row, 0), name)
        self.model.setData(self.model.index(row, 1), address)
        self.model.setData(self.model.index(row, 2), rssi)
        
    def onScanFinished(self):
        print("BLE 스캔이 완료되었습니다.")
        # 필요시 UI 처리
    
    def accept(self):
        
        try:
            index = self.tableViewDeviceList.selectedIndexes()[0]
            
            row = index.row()
            name = self.model.item(row, 0).text()
            address = self.model.item(row, 1).text()
            
            if address in self.device_info_map:
                device_info = self.device_info_map[address]
                print(f"선택한 장치: {device_info.name()} ({address})")
                self.deviceSelected.emit(device_info)
            
            # #스캔중이라면 스캔중지
                self.agent.stop()
                
        except Exception as e:
            print(e)
        finally:
            return super().accept()
        

    def closeEvent(self, event):
        self.ble_scanner.stop()
        self.ble_scanner.wait()
        super().closeEvent(event)
        
if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    dlg = DlgDeviceList()
    
    dlg.deviceSelected.connect(lambda name, address: print(f"select device {name} : {address}"))  # 선택된 디바이스 출력
    dlg.show()
    app.exec()
    