# 硬件驱动来源

上游：https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.85B

- 屏幕、触摸、I2C、LVGL 适配和 lv_conf.h 来自 `examples/Arduino-V3.2.0/examples/01_lvgl_demo/`。
- es8311.cpp、es8311.h、es8311_reg.h 来自 `examples/Arduino-V3.2.0/examples/03_audio_out_no_tf/`。
- `LVGL_Driver.cpp` 改用与部分帧缓冲兼容的刷新方式，并在传输完成后通知 LVGL；头文件移除未使用的 demos 引用。
- `Display_ST77916.cpp/.h` 增加异步传输完成回调、将最大传输长度调整为 LVGL 部分帧缓冲大小、释放探测用 SPI IO 句柄、移除启动测试图，初始化屏幕识别寄存器缓冲区。
- `Touch_CST816.cpp` 初始化读取缓冲区，避免读取失败时使用未初始化数据。
- `lv_conf.h` 关闭不使用的演示，保留 canvas。
- 其余应用和音频/IMU接入代码为本工程新增。

下载时的上游提交记录在 `UPSTREAM-REVISION.txt`。
