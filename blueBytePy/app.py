# 파일명 app.py
import sys
from PySide6.QtWidgets import QApplication
from mainWindow import MainWindow

try :
    app = QApplication(sys.argv)
    widget = MainWindow()
    widget.show()
    sys.exit(app.exec())
except Exception as e:
    print(e)
    sys.exit(1)
    