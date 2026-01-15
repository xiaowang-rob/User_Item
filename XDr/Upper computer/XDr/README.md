## 代码结构

- `start.py`：主程序，运行此文件即可运行整个项目。
- ui
  - `main_window.py`：主界面，负责显示所有功能。
  - `UI_XDr.py`:qtdesigner生成的UI文件，负责设计UI。
  - `slots.py`：槽函数，负责处理UI的事件。
- functions
  - `com_port.py`：串口通信 负责连接com端口、发送数据、接收数据。
  - `data_process.py`：数据处理，包括数据采集、数据清洗、数据转换等。
  - `parameter.py`: 参数管理，包括串口参数设置、数据处理参数设置等。