# 又又和小A

给又又的四岁生日礼物：照顾银渐层小猫小A，一起开太空车、看深圳车流、装饰生日蛋糕。当前交付为 **V8**，固件工程名与烧录文件名保留为 `YouyouSpace`。

目标开发板：**Waveshare ESP32-S3-Touch-LCD-1.85B**，360×360圆形触摸屏、16MB Flash、8MB OPI PSRAM。请核对型号末尾的 **B**，其他1.85英寸板卡不能直接视为兼容。

游戏离线运行，无需Wi-Fi、账号或联网校时。SD卡用于可选录音，不是游戏与存档的必需品。

## 目录

- [快速开始与交付内容](#快速开始与交付内容)
- [Windows烧录预编译固件](#windows烧录预编译固件)
- [烧录异常与完全断电重启](#烧录异常与完全断电重启)
- [首页与返回操作](#首页与返回操作)
- [照顾小A](#照顾小a)
- [成长规则与需要提示](#成长规则与需要提示)
- [房间纪念物](#房间纪念物)
- [星球探险生日派对与贴纸](#星球探险生日派对与贴纸)
- [像素深圳](#像素深圳)
- [SD卡与录音](#sd卡与录音)
- [存档与升级](#存档与升级)
- [源码构建](#源码构建)
- [预览素材与测试](#预览素材与测试)
- [验证范围与实板验收](#验证范围与实板验收)
- [项目结构与清理约定](#项目结构与清理约定)
- [资料与许可](#资料与许可)

## 快速开始与交付内容

只需要烧录时，下载并**完整解压** [V8交付包](dist/YouyouSpace-v8.zip)，按下一节操作，不需要Arduino IDE或重新生成图片。

| 文件 | 用途 |
|---|---|
| `dist/flash-windows.cmd` | Windows烧录入口，需要Python 3和Python Launcher |
| `dist/YouyouSpace.bootloader.bin` | ESP32-S3引导程序 |
| `dist/YouyouSpace.partitions.bin` | 本项目16MB分区表 |
| `dist/boot_app0.bin` | OTA启动选择数据 |
| `dist/YouyouSpace.app.bin` | V8游戏程序 |
| `dist/FLASH-PLAN.txt` | 镜像地址、长度和NVS保留范围 |
| `dist/SHA256SUMS.txt` | 包内源码、资源、镜像与说明的SHA256清单 |
| `dist/BUILD-STATUS.txt` | 编译结果、固件摘要与验证边界 |
| `dist/PERFORMANCE.txt` | 电脑端绘图与传输像素基准 |

预览入口：

- [36秒完整交互演示 GIF](dist/pet-room.gif) / [MP4](dist/pet-room.mp4)：需要提示、喂食、成长小花、呼唤回应、城市纪念车与洗澡。
- [20秒三阶段养宠演示 GIF](dist/pet-care.gif) / [MP4](dist/pet-care.mp4)：三份独立的幼崽、小猫、成猫示例存档，不表示20秒能长成成猫。
- [24秒像素深圳 GIF](dist/pixel-shenzhen.gif) / [MP4](dist/pixel-shenzhen.mp4)：一段完整循环，GIF无限播放，MP4需播放器开启循环。
- [20个场景总览](dist/preview.png)、[行走截图](dist/pet-walk.png)、[纪念物齐全的小窝示例](dist/pet-memories.png)、[深圳截图](dist/pixel-shenzhen.png)。纪念物齐全示例预置完成记录，只展示奖励位置。

以上媒体由实际游戏与绘图代码在电脑上生成，无音频，不是开发板实拍。

## Windows烧录预编译固件

### 1. 准备环境

1. 使用Windows 10/11，安装Python 3，建议使用python.org的64位Python 3.11或3.12，并安装 **Python Launcher**。
2. 完整解压交付包。不要在ZIP压缩包预览窗口内直接运行脚本。
3. 使用支持数据传输的USB Type-C线，连接开发板与电脑；关闭Arduino串口监视器、串口调试助手等占用端口的软件。
4. 打开设备管理器的“端口（COM和LPT）”，记录开发板对应端口，例如`COM5`。插拔对比可帮助确认，重新进入BOOT下载模式后端口可能变化。

可在命令提示符先检查：

```bat
py -3 --version
```

本交付包仍使用 **Python + esptool 4.8.1** 方案，**未包含Windows离线exe烧录器**。脚本会检查工具版本，缺失或版本不符时通过pip安装固定版本，因此首次准备工具需要联网。遇到下载失败，应先解决Python/pip网络问题再烧录。

### 2. 双击脚本

1. 打开解压后的`dist`目录，双击`flash-windows.cmd`。
2. 输入实际端口，例如`COM5`。脚本接受首尾空白和小写输入，规范化后检查端口格式。
3. 等待esptool完成写入与数据校验。脚本默认使用`115200`波特率。
4. 看到`Flash write and verification completed`表示工具报告写入和校验完成，**不等于已经确认游戏成功启动**。随后观察屏幕是否进入“又又和小A”首页。
5. 若使用过BOOT强制下载，按下一节完成断电重启，松开BOOT后再正常上电。

### 3. 实际烧录地址

脚本自动使用以下地址，无需手工修改：

| 地址 | 文件 | 本次镜像长度 |
|---|---|---:|
| `0x000000` | `YouyouSpace.bootloader.bin` | 20,208字节 |
| `0x008000` | `YouyouSpace.partitions.bin` | 3,072字节 |
| `0x00E000` | `boot_app0.bin` | 8,192字节 |
| `0x010000` | `YouyouSpace.app.bin` | 2,703,968字节 |

这些镜像的写入范围和向外取整的擦除扇区均避开本项目NVS：`0x009000–0x00DFFF`。不要使用整片擦除选项。

也可在`dist`目录手工执行，替换真实端口：

```powershell
py -3 -m pip install esptool==4.8.1
py -3 -m esptool version
py -3 -m esptool --chip esp32s3 --port COM5 --baud 115200 write_flash 0x0 YouyouSpace.bootloader.bin 0x8000 YouyouSpace.partitions.bin 0xe000 boot_app0.bin 0x10000 YouyouSpace.app.bin
```

不要把单独的应用镜像写到`0x0`。微雪官方出厂合并固件写`0x0`的说明，不等于本项目四个分段文件都写`0x0`。

### 4. 可选：检查解压后的文件完整性

在解压后的`YouyouSpace`项目根目录打开PowerShell运行：

```powershell
Get-Content -Encoding UTF8 .\dist\SHA256SUMS.txt | ForEach-Object {
    $parts = $_ -split '  ', 2
    $path = Join-Path (Get-Location).Path $parts[1]
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $parts[0]) {
        throw "SHA256 mismatch: $($parts[1])"
    }
}
```

没有报错表示与清单一致。SHA256用于发现文件损坏或版本混用；修改源码后需要重新生成交付清单。

## 烧录异常与完全断电重启

| 现象 | 优先检查 |
|---|---|
| `py`或Python找不到 | 安装Python 3及Windows Python Launcher，确认`py -3 --version`成功 |
| pip/esptool安装失败 | 检查网络与Python环境；手动安装固定版本4.8.1，再运行脚本 |
| 提示缺少镜像 | 完整解压整个交付包，四个BIN与CMD应在同一个`dist`目录 |
| 找不到COM口 | 换数据线或电脑USB口；确认板卡供电，在设备管理器重新查看 |
| `Access is denied`、端口被占用 | 关闭串口监视器和其他串口程序 |
| `Failed to connect`、持续`Connecting...` | 确认端口；必要时完全断电后按住BOOT重新上电进入下载模式 |
| 写入校验不通过 | 检查文件SHA256、数据线和供电，保留稳定波特率，不要反复整片擦除 |
| 写入成功但仍不启动 | 松开BOOT，完全断电重启；BOOT下载前后的COM口可能不同 |
| 黑屏、白屏或反复重启 | 核对完整型号1.85B、四个镜像地址及OPI PSRAM配置；查看115200串口日志 |
| `Canvas allocation failed` | PSRAM未正确启用或分配失败；核对OPI配置与板卡型号 |
| 无声音 | 缺录音时应有内置音效；再检查喇叭是否正确接入、文件格式和串口音频初始化日志 |

**完全断电包括USB和电池供电。** 微雪说明：接着电池时只拔USB不一定断电；连接USB时长按PWR，也不能一概认为已经完全断电。

需要强制下载时：确保设备完全断电，按住BOOT，再给开发板上电，待端口出现后松开BOOT并重新确认COM口。烧录结束后释放BOOT，再完全断电重启。若仍由电池供电，应在拔USB后通过PWR关机；不确定是否已断电时，按微雪板卡说明处理电池连接，勿把仅拔USB当作充分条件。

默认先尝试脚本自动连接；不需要为了正常更新而每次强制进入BOOT模式。板卡的PWR与其他开发板常见的EN/RESET按钮不是同一概念。

目前未在Windows实机执行该CMD，也未连接本项目开发板实烧，因此不能承诺任意线材、端口和板卡状态下一次成功。

## 首页与返回操作

首页有“小A的家”“星球探险”“生日派对”，上方还有贴纸和“像素深圳”入口。

- **房子图标 + 首页**：回到总菜单。
- **爪印图标 + 小窝**：从照顾活动回到小A房间。
- 房间里的四个卡片是喂鱼、洗澡、玩耍、铲屎；右侧月亮是睡觉入口。
- 右上角的大图标显示当前一种需要，点击可直接开始对应活动；状态良好时邀请摸摸。其他入口仍然可用，不强制按提示顺序玩。

所有主要操作均支持点击。驾驶还支持拖动与可用时的倾斜控制，摸摸也可轻轻划动身体；不要求联网、语音识别或复杂手势。

## 照顾小A

| 活动 | 操作和反馈 | 完成条件 |
|---|---|---|
| 喂鱼 | 点鱼/食盆、嘴边或大按钮；鱼逐口变小，有送入口中与开心反馈 | 三口完成一轮；一轮开始时饱腹达到85及以上会提示吃饱，可点球图标或按钮去玩 |
| 洗澡 | 点泡泡原位消除；随后点水滴区域/大按钮冲洗，再点毛巾区域/大按钮擦干 | 四个泡泡、三次冲洗、三次擦干 |
| 玩耍 | 点移动小球，小球弹起并出现星点，小A跟着球移动 | 一轮命中五次；精力不高于25时提示休息，可点月亮进入睡觉 |
| 铲屎 | 点对应的便便，出现铲走与亮点反馈 | 清理当前所有便便；没有便便时不会重复发奖励 |
| 睡觉 | 点月亮；睡眠中轻点猫咪或“轻轻叫醒”可回窝 | 开机时每三秒恢复一点精力 |
| 摸摸 | 点移动中的小A或轻轻挠身体，会停下来并出现爱心 | 有短冷却，增加心情，不需要完成任务 |

未命中喂食、冲洗、擦干或小球目标时，不推进任务。部分操作会提示目标位置；大按钮作为容易点中的替代入口。重复照顾已经干净或开心的小A仍可获得动作回应，但不一定再次增加成长值。

小A会在房间前景地面散步、停留。点击地毯或空地可以叫它过去，到达后会停下等待摸摸；空闲时偶尔停下来注视又又。触摸范围跟随位置和体型，移动不遮住主要状态区和照顾按钮。

走路使用左右各八帧四足姿态、三个转身角度和左右站立姿态，共21张预处理素材。步态按实际位移推进；起步先站立再加速，接近目标减速，停稳后转向，休息时再坐下。喂鱼、洗澡、铲屎和睡觉保持对应位置。

房间背景采用180×180逻辑像素整数放大；云朵、绿植和阳光轻微变化，猫咪保持银渐层玩偶外观。离开小猫页面暂停走动计算，养成状态仍按设备开机时间推进。

## 成长规则与需要提示

### 当前需要的优先级

1. 精力低于40：想睡觉。
2. 有便便：一起清理。
3. 饱腹低于65：想吃小鱼。
4. 清洁低于65：想洗香香。
5. 心情低于70：想玩小球。
6. 以上都满足时：想要摸摸。

这是邀请顺序，不是强制任务清单。

### 成长与状态

- 成长阶段：**幼崽0–59 → 小猫60–179 → 成猫180以上**，成长值上限240；不会因一时忘记照顾而倒退。
- 幼崽和小猫阶段用六朵小花表示当前阶段进度，获得成长后逐步亮起；进入成猫阶段后六朵花常亮。不是每一次点击都增加一朵花。
- 有效完成喂鱼、洗澡、玩耍、铲屎，可分别获得6、8、6、6点成长。同类成长奖励有180秒开机活动时间冷却，并要求本轮开始时确实有需要。
- 喂食成长条件为一轮开始时饱腹低于85；洗澡为清洁低于85；玩耍为心情低于90且有足够精力；铲屎为确实有便便。重复完成仍可开心回应，完成页会区分成长奖励与普通陪伴。
- 状态良好时，每累计一分钟补充一点成长：饱腹至少55、清洁至少60、心情至少55、精力至少45、便便不超过一块。
- 每分钟饱腹减少2；清洁在有/无便便时分别减少3/1；心情相应减少2/1；非睡眠状态每分钟精力减少1。四种状态最低20、最高100。
- 吃下一轮的第一口后即进入消化计时，约150秒产生一块便便，最多三块。未完成整轮就离开，已经吃下的食物仍保留。
- 只计算设备开机时间，包括菜单、驾驶和城市模式；关机暂停，不追算离线时间。

完成后会显示“成长花开啦”或正面的陪伴反馈；已经很干净的重复洗澡会说明“已经香香的啦，也很开心”。

## 房间纪念物

| 行为 | 留在房间的变化 |
|---|---|
| 首次完整喂鱼 | 小鱼纪念装饰 |
| 首次完成洗澡 | 小花 |
| 首次完成玩球 | 玩具篮里的小球 |
| 喂鱼、洗澡、玩球、铲屎四种活动均完成 | 相框中的小A照片 |
| 完成任意太空驾驶任务 | 月亮海报与玩具车 |
| 在深圳模式点击道路叫小A的小车 | 玩具车；已有玩具车时不重复创建 |
| 完成生日许愿 | 气球 |

回小窝发现新的纪念物时，会短暂显示提示，并在对应位置闪星。纪念物由完成记录和贴纸推导，重启后可恢复，不需要每帧写Flash。

## 星球探险生日派对与贴纸

### 三个星球

没有完成过驾驶任务时，进入探险会直接开始月亮采星；完成一次后，后续进入可选择三个星球。

| 任务 | 目标 |
|---|---|
| 月亮采星 | 收集五颗星星 |
| 火星救援 | 先收集三个能量，再接到小A救援舱 |
| 彩环寻宝 | 按提示颜色依次收集五颗星星 |

左右拖动或点击控制小车；IMU可用时也可倾斜，触摸操作会暂时接管方向。碰到石头会短暂减速，磁铁能吸引收集物；没有生命扣光的失败结束。完成后可“选星球”或去“下一站”。

### 生日派对

选择草莓、星星或小鱼装饰，点击蛋糕放置；再点击“点蜡烛”，逐根点亮四根蜡烛。进入许愿阶段后，逐根点击熄灭，播放生日祝福或内置生日旋律，并解锁烟花。派对中点空处可放烟花，下方按钮可重新开始。

当前使用点击蜡烛，**未实现麦克风吹气识别**；生日流程可随时重玩，不依赖硬件日期或联网校时。

### 六张贴纸

| 贴纸 | 条件 |
|---|---|
| 小A好朋友 | 完成四种照顾活动 |
| 月亮探险家 | 完成月亮任务 |
| 火星救援队 | 完成火星任务 |
| 彩环收藏家 | 完成彩环任务 |
| 生日魔法师 | 完成生日许愿 |
| 追球小高手 | 累计命中小球五次 |

贴纸与养成记录一起保存；点击贴纸页面可查看对应名称和提示。

## 像素深圳

六处地标同时出现：平安金融中心、京基100、地王大厦、华润春笋、深圳市民中心、深圳湾体育中心“春茧”。这是适配小屏幕的组合剪影，不按真实地理位置排列。

- 明亮糖果色白天场景：笑脸太阳、浮云、海湾波纹、薄荷和浅紫建筑。
- 六条双向交通车道与斜向高架持续行车，有轿车、出租车、巴士和小货车。
- 每四秒自动高亮一处地标；点击建筑可固定，点击名称条恢复轮播。
- 点击下方道路可叫小A的小车驶过，并回应猫叫；一次行程未结束时不重复创建，第一次可获得房间玩具车。
- 无分数与失败，可一直观看。基础车流、云朵、波纹和地标轮播为24秒闭合周期，重新进入从头开始；点击叫车是额外的短动画，不要求它与基础循环同周期。

## SD卡与录音

将FAT32 Micro SD卡中的`youyou`目录放在卡根目录，即`/youyou/`，不要多嵌套一层`sd-card`。文件名使用下表的小写英文。

| 文件 | 播放时机 | 可参考的录音内容 |
|---|---|---|
| `hello.wav` | 每次开机 | 又又你好，我是小A，今天一起玩吧 |
| `meow.wav` | 打开小猫、摸摸或喂食等 | 0.3–1秒短猫叫 |
| `birthday.wav` | 许愿完成 | 家人的生日祝福，建议10–30秒 |
| `feed.wav` | 进入喂鱼 | 点小鱼，喂小A吃一口 |
| `bath.wav` | 进入洗澡 | 点泡泡，给小A洗香香 |
| `play.wav` | 进入玩耍 | 点小球，陪小A一起玩 |
| `litter.wav` | 进入铲屎 | 点便便，帮小A清理小窝 |
| `sleep.wav` | 进入睡觉 | 小A要睡觉啦，轻轻点它可以叫醒 |

格式必须是 **WAV、PCM signed 16-bit、单声道、16000Hz**。把MP3/M4A改扩展名不会转换格式。可用Audacity导出，或FFmpeg转换：

```powershell
ffmpeg -i "爸爸妈妈的祝福.m4a" -ac 1 -ar 16000 -c:a pcm_s16le birthday.wav
```

录音模板和台词见 [录音说明](sd-card/youyou/录音说明.txt)。交付包不包含家人的录音，也没有默认中文语音；没有卡或对应文件时，仍可玩游戏，改用内置短音效或生日旋律。换卡、添加文件后重新开机挂载。

## 存档与升级

自动保存：四种状态、成长、便便、待消化食物、活动时间、照顾记录、成长奖励冷却、贴纸和城市纪念车标志。

不保存：正在进行的喂食口数/洗澡步骤/玩球轮次、驾驶关卡进度、蛋糕装饰、猫咪位置/朝向/步态、当前页面和城市临时叫车。重启后从首页开始，已吃下的食物与已取得的持久结果保留。

普通变化合并后自动写入，首次记录约等待1.8秒，已有记录的普通写入间隔至少5秒；返回小窝或离开照顾页面时可立即尝试提交。关机前建议先回小窝并等待约5秒。写入失败会继续重试，不显示已经保存的假确认。

计时字段随实际记录一起提交，不是每秒写Flash；突然断电可能丢失最后一段尚未提交的变化。

记录为96字节，包含版本、序号和校验；A/B两份交替写入，启动时选择较新的有效记录，一份损坏时可退回另一份；两份都无效时恢复初始小A，并尝试保留旧版贴纸。可读取V4以后版本的有效养成记录；V2/V3旧版只迁移贴纸。V8扩展了完成标志中的城市纪念位，不保证旧固件降级后能读取新版标志。

NVS位于板载Flash，不需要SD卡。当前分段升级方案避开NVS，但这以本项目相同分区布局为前提；覆盖其他厂商/其他分区固件时，不能据此承诺保留对方数据或出厂功能。

## 源码构建

### Arduino IDE：Windows

1. 安装Arduino IDE 2，在“文件→首选项→附加开发板管理器网址”添加：

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

2. 安装 **esp32 by Espressif Systems 3.2.0** 与 **lvgl 8.4.0**。本项目按这些固定版本编译，不混用LVGL 9。
3. 查看Arduino首选项中的“项目文件夹位置”。将`firmware/YouyouSpace/lv_conf.h`复制到该位置下的`libraries/lv_conf.h`，例如`Documents/Arduino/libraries/lv_conf.h`，保留工程内原文件。
4. 打开`firmware/YouyouSpace/YouyouSpace.ino`，选 **ESP32S3 Dev Module**。
5. 使用以下配置：

   | 设置 | 值 |
   |---|---|
   | Flash Size | 16MB (128Mb) |
   | Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
   | PSRAM | OPI PSRAM |
   | USB CDC On Boot | Enabled |
   | USB Mode | Hardware CDC and JTAG |
   | CPU Frequency | 240MHz |

6. 选择真实COM口上传；连接失败按前述BOOT与完全断电流程处理。

显示、触摸和ES8311驱动已随工程提供，不需要再安装SensorLib或ESP32-audioI2S。音频使用Arduino-ESP32自带ESP_I2S。正式构建直接使用已生成的字体和图像C数组，不需要Pillow或imagegen。

### Arduino CLI：Linux

先安装Arduino CLI，并安装固定依赖：

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.2.0 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install 'lvgl@8.4.0'
bash tools/build-linux.sh
```

构建脚本使用FQBN：

```text
esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc
```

可用`ARDUINO_CLI=/path/to/arduino-cli`选择CLI路径，依赖应已装在该CLI实际使用的目录。脚本会生成并校验四个分段镜像，默认清理`dist/build/`和中间的全量`YouyouSpace.bin`。调试时可保留ELF等完整输出：

```sh
YOUYOU_KEEP_BUILD=1 bash tools/build-linux.sh
```

全量合并镜像用于开发恢复，可能重置存档，不是默认升级文件，也不放进交付ZIP。

## 预览素材与测试

### 电脑绘图预览与六组测试

需要CMake、C/C++编译器和LVGL 8.4.0源代码。运行以下命令，替换LVGL源码路径：

```sh
cmake -S tools -B /tmp/youyou-preview -DLVGL_DIR=/path/to/lvgl-8.4.0
cmake --build /tmp/youyou-preview -j8
ctest --test-dir /tmp/youyou-preview --output-on-failure
mkdir -p /tmp/youyou-shots
/tmp/youyou-preview/youyou_preview /tmp/youyou-shots
python3 tools/make_preview_sheet.py --input-dir /tmp/youyou-shots
```

拼图脚本需要Pillow和Noto Sans CJK Bold字体，当前字体路径为Linux的`/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc`。默认只更新`dist/preview.png`，不向项目导出20张重复PNG。确实需要逐张检查时：

```sh
python3 tools/make_preview_sheet.py --input-dir /tmp/youyou-shots --export-scenes /tmp/youyou-scenes
```

| 测试目标 | 覆盖内容 |
|---|---|
| `game_test` | 菜单、驾驶、蛋糕与贴纸回归 |
| `pet_test` | 养成边界、奖励冷却、消化与存档校验/备用恢复 |
| `city_test` | 周期、车距、地标点击和缓冲边界 |
| `walk_test` | 十万步走动边界、距离步态关系、起停转向和移动触摸区域 |
| `child_test` | 需要提示、操作目标、背景点击无效、语音事件、呼唤回应与纪念物存档 |
| `view_test` | 缓存绘图与完整绘图逐像素一致，实际LVGL局部flush重建与传输基准 |

### 动态媒体

预览程序将PPM帧流写到标准输出。以新版互动视频为例，需先安装FFmpeg：

```sh
/tmp/youyou-preview/room_preview | ffmpeg -f image2pipe -vcodec ppm -framerate 50/3 -i - -c:v libx264 -pix_fmt yuv420p -movflags +faststart -y dist/pet-room.mp4
```

- `room_preview`：600帧，50/3fps，36秒；附带任意参数时只输出一帧预置纪念物房间。
- `city_preview`：480帧，20fps，24秒；附带任意参数时只输出首帧。
- `animation_preview`：1000帧，50fps容器，20秒；三份独立成长阶段的养宠示例，实际绘图遵循场景刷新间隔。

### 素材再生成与打包

仅修改素材或文字时才需要以下工具：

| 工具 | 输入与输出 |
|---|---|
| `pack_art.py` | `assets/v4/silver-space-atlas.png`与独立蛋糕原图 → `ArtAssets.c/h` |
| `pack_pet_art.py` | `assets/v4/pet-atlas.png` → `PetAssets.c/h` |
| `pack_walk_art.py` | `assets/walk`的行走/转身原图 → `WalkAssets.c/h` |
| `make_font.py` | 界面中文子集与Noto Sans CJK Bold → 两个字体C文件 |
| `prepare_flash_files.py` | Arduino编译目录与合并镜像 → 四个分段镜像、NVS扇区检查与烧录计划 |
| `make_release.py` | 当前源码、原图、主要预览、镜像与说明 → V8 ZIP及SHA256清单 |

修改源码后，先重新编译固件、按需更新预览和`BUILD-STATUS.txt`，再运行`python3 tools/make_release.py`。打包脚本本身不编译固件，也不能判断现有BIN是否对应刚修改的源码。

原图和完整生成提示词保留在`assets/v4`与`assets/walk`。PNG仅是素材制作输入；设备直接读取RGB565+A8数组，不进行运行时PNG解码或镜像转换。

## 验证范围与实板验收

当前固件由Arduino-ESP32 3.2.0 / LVGL 8.4.0编译通过；程序统计2,703,822字节，约占3MB应用分区的86.0%，编译器统计全局变量128,756字节。应用BIN为2,703,968字节，与程序统计的口径不同，不是文件损坏。

已验证：六组电脑测试、儿童交互及步态的ASan/UBSan检查、镜像内置校验和/验证哈希、交付包SHA256，以及分段写入/擦除范围避开NVS。BOOT镜像解析为ESP32-S3、16MB Flash、80MHz、DIO头部；它与Arduino所用启动配置配套，不需要凭其他型号教程手动覆盖。

电脑端固定基准：

| 场景 | 平均实际flush像素/帧 | 占360×360整屏比例 |
|---|---:|---:|
| 驾驶 | 44,090 | 34.0% |
| 像素深圳 | 50,988 | 39.3% |
| 小窝 | 10,641 | 8.2% |

游戏逻辑目标50Hz，驾驶绘图间隔33ms，小窝60ms，睡眠100ms，城市50ms；这些是调度目标，不是开发板实测帧率。绘图复用三块RGB565画布共777,600字节PSRAM，按30×30变化块合并传输；音频独立任务处理，操作不逐帧分配对象或写存档。

串口115200每10秒打印一次`PERF`，包含绘图次数、平均绘图耗时、变化区域像素、可用堆和PSRAM。空闲首页`frames=0`可以是正常现象。

**尚未完成实板验收**。烧录后按以下顺序检查：

1. 能稳定进入首页，触摸位置与画面对应。
2. 需要图标和四个照顾卡片容易点中；小A走动、呼唤、摸摸没有明显残影。
3. 完整体验喂鱼、洗澡、玩球和铲屎，验证成长花与纪念物。
4. 回小窝等待约5秒后完全断电重启，确认成长、便便、贴纸与纪念物仍在。
5. 确认开机问候、提示录音和生日祝福；没有SD卡也能正常运行。
6. 试玩三种驾驶任务、蛋糕许愿和深圳地标/叫车，观察流畅度及长时间运行内存。

## 项目结构与清理约定

```text
README.md                  当前实现、操作、构建和验证说明
firmware/YouyouSpace/       Arduino入口、游戏模型、显示/触摸/音频驱动及生成的C素材
firmware/*LICENSE*         上游驱动与字体许可
assets/v4/                 三张原始素材图、提示词和打包清单；仍被当前版本使用
assets/walk/               两张行走/转身原图、完整提示词和打包清单
tools/                     构建、素材转换、预览、测试和打包工具
sd-card/youyou/             SD目录模板及录音说明
dist/                      最新分段固件、主要预览、说明、校验清单及V8交付包
```

`Game.h`管理总场景；`PetModel.h`管理照顾与需要，`PetWalk.h`管理临时走动，`PetSave.h`管理记录；`SceneView.cpp`与`DirtyTiles.h`负责渲染和局部刷新；`PixelRoom.h`与`PixelCity.h`绘制像素场景；`BoardExtras.cpp`接入音频、SD和IMU。

当前只保留最新版交付。20张单独场景PNG已合并保留在总览图中，旧构建机专用下载脚本、临时下载包、PPM和编译中间产物不属于交付内容。GIF和MP4分别服务直接预览与视频播放，原始素材与生成的C数组分别服务素材制作和编译，均有保留用途。

同一份`flash-windows.cmd`在`tools`中作为源文件，在`dist`中作为可直接执行的交付副本；不要只修改其中一份。更新交付包后同步重建SHA256清单。

## 资料与许可

- [微雪1.85B产品文档](https://docs.waveshare.net/ESP32-S3-Touch-LCD-1.85B/)：型号、资源和引脚。
- [微雪Arduino开发](https://docs.waveshare.net/ESP32-S3-Touch-LCD-1.85B/Arduino/)：官方开发资料。本项目仅使用自身实际需要的驱动和固定依赖。
- [微雪FAQ](https://docs.waveshare.net/ESP32-S3-Touch-LCD-1.85B/FAQ/)：BOOT下载、串口占用与USB/电池供电注意。
- [Espressif esptool v4命令](https://docs.espressif.com/projects/esptool/en/release-v4/esp32s3/esptool/basic-commands.html)：镜像地址、写入和扇区擦除。
- 硬件驱动来源：[微雪官方仓库](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.85B)。上游版本与适配说明见 [UPSTREAM.md](firmware/UPSTREAM.md)、[UPSTREAM-REVISION.txt](firmware/UPSTREAM-REVISION.txt)，许可见 [WAVESHARE-LICENSE](firmware/WAVESHARE-LICENSE)。
- 中文字体为Noto Sans CJK Bold子集，许可见 [FONT-LICENSE.txt](firmware/FONT-LICENSE.txt)。
- 地标外形参考：[深圳档案资料](https://www.szdag.gov.cn/gzdt/gzdt/content/post_1590841.html)、[深圳城市天际线](https://www.sz.gov.cn/cn/xxgk/zfxxgj/tpxw/content/post_12310079.html)、[地名资料](https://pnr.sz.gov.cn/attachment/1/1442/1442495/10537363.pdf)。
