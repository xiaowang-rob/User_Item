```
git上传
1.上传至git暂存区
git add file.xxx 单个文件
git add . 多个文件
2.提交到本地仓库
git commit -m"提交信息说明"
3.推送到Github仓库
git push origin main (origin 远程仓库默认别名 main 本地分支名称)
```
```
git覆盖
1.强制覆盖
git pull origin 分支名称 --force(强制)
```
# 嵌入式学习记录
keil5代码和嘉立创EDA文件

### 1.Nono智能时钟
1. 已经可以通过ESP32WiFi模块获取时间并显示
2. 还有充电模块正在改
3. 蓝牙调试连WiFi 换壁纸 等
4. 语音模块
### 2.Dog自制四足
1. 第一版腿太软 舵机力不足
2. 计划改小改轻
### 3.voicechange硬件变声器
1. 板子搞定
2. usb识别搞定
3. INMP441 接收PCM数据24位有效32位数据
4. 环形缓冲区搞定
5. flash正在搞
6. 数据传到电脑还有点问题 感觉没进USBFS中断