# ESP32 TEN-Agent 功能说明

本说明帮助快速理解本项目的整体架构、关键流程和可扩展接口，便于后续二次开发（含音视频编解码或上层应用对接）。

## 架构概览
- 设备启动后按顺序执行：初始化 NVS → 连接 Wi‑Fi → `ai_agent_generate` 通过 HTTP 获取 TEN-Agent RTC 鉴权 → 初始化按键、音频、视频 → 使用 appId/token 通过 Agora IoT SDK 入会 → 按键控制向 TEN-Agent 发送 `start/stop/ping` 请求。
- 音频链路：麦克风 → ADF 管线（I2S 采集 + AEC `algorithm_stream`）→ 原始 PCM → `send_rtc_audio_frame` 上行；下行音频在回调写入播放 `raw_stream`。
- 视频链路（非 `CONFIG_AUDIO_ONLY`）：摄像头采集 YUV422 → 软件 JPEG 编码 → `send_rtc_video_frame` 上行；帧间隔默认 200 ms。
- TEN-Agent HTTP 接口：`token/generate` 获取 appId/token，`start` 选择 graph 并传入 RTC 编码参数，`ping` 保活，`stop` 终止；响应解析填充全局 `g_app` 状态驱动 RTC。

### 拓扑图（逻辑链路）
```
[Mic] --I2S--> [ADF 采集+AEC] --PCM--> [Agora RTC 上行] --(Ten-Agent Graph)--> [LLM/TTS]
   ^                                               |
   |                                               v
[Speaker] <--PCM-- [Agora RTC 下行] <---(生成语音)-- [Ten-Agent]

[Camera] --YUV--> [JPEG 编码] --Frame--> [Agora RTC 上行] --(Ten-Agent Graph)--> [LLM/Vision]
```

## 关键配置
- `main/app_config.h`
  - `TENAI_AGENT_URL`：TEN-Agent 服务地址（通常 8080）。
  - Graph 选择：`CONFIG_GRAPH_OPENAI`（仅音频）或 `CONFIG_GRAPH_GEMINI`（音视频），宏决定 `GRAPH_NAME`。
  - 模型/语音/语言等参数通过 `V2V_MODEL`、`VOICE`、`LANGUAGE`、`GREETING`、`PROMPT`。
  - 音频编解码选择：默认 `CONFIG_USE_G711U_CODEC`（8 kHz）；切换宏可用 G.722 或自定义。
- `main/common.h`
  - 全局状态 `app_t` 与音频参数常量（采样率、帧长、payload type），`TENAI_AUDIO_CODEC` 通过 HTTP 传给 TEN-Agent，对应 RTC payload type。
- `main/Kconfig.projbuild`
  - menuconfig 中配置 Wi‑Fi SSID/PWD、CPU 频率等。

## 文件职责速查
- `README.md` / `README.cn.md`：硬件需求、环境准备、编译/烧录步骤。
- `main/CMakeLists.txt`：组件依赖声明（esp-adf、esp32-camera、agora_iot_sdk 等）。
- `main/app_config.h`：TEN-Agent URL、Graph/模型参数、频道/用户 ID、编解码宏开关。
- `main/common.h`：全局状态结构、音频采样/帧长/codec 常量、TEN-Agent codec 参数。
- `main/ai_agent.c` + `ai_agent.h`：TEN-Agent HTTP 客户端，构造 JSON（generate/start/ping/stop），解析响应并填充 `g_app`。
- `main/rtc_proc.c` + `rtc_proc.h`：Agora IoT SDK 适配，事件回调、入会/退会、音视频帧发送。
- `main/audio_proc.c` + `audio_proc.h`：ADF 音频采集/播放管线，AEC 处理，上行 PCM 发送，下行播放，音量接口。
- `main/video_proc.c` + `video_proc.h`：摄像头引脚与分辨率配置，JPEG 编码，视频帧采集/发送线程。
- `main/llm_main.c`：应用入口，Wi‑Fi/按键初始化，启动音视频任务，调用 TEN-Agent generate/start/stop/ping，主循环打印内存。
- `main/Kconfig.projbuild`：menuconfig 选项（Wi‑Fi、CPU 频率）。
- `partitions.csv` / `sdkconfig*`：分区和编译配置（由 menuconfig/工具生成，通常无需手改）。

## 模块职责
- `main/ai_agent.c`：封装 TEN-Agent HTTP 交互。构建 JSON 请求、发送 `generate/start/ping/stop`，解析 `{code,msg,data}` 提取 appId/token 并更新 `g_app`。
- `main/rtc_proc.c`：Agora IoT SDK 适配。注册事件、`agora_rtc_join_channel` 入会、`send_rtc_audio_frame/send_rtc_video_frame` 发送媒体、错误打印。
- `main/audio_proc.c`：ADF 音频管线。Recorder：I2S → AEC → raw；Player：raw → I2S。`audio_send_thread` 在会话期间持续推流，回调写入下行数据。
- `main/video_proc.c`：摄像头与 JPEG 编码。采集、编码、发送循环；分辨率/质量可按带宽调整。
- `main/llm_main.c`：应用入口。Wi‑Fi 事件、按键控制（SET 启动 Agent，MUTE 停止，VOL± 调音量）、启动音视频任务、定时 `ai_agent_ping` 保活并打印内存。

## 运行时序（便于对接）
1) 配置 `main/app_config.h`（TEN-Agent URL、频道名、Graph/模型参数），`idf.py menuconfig` 设置 Wi‑Fi。  
2) 上电后 `ai_agent_generate` 调用 `token/generate`，成功后保存 appId/token。  
3) 音频线程启动并等待 RTC 入会；`agora_rtc_proc_create` 使用 token 入会并订阅下行。  
4) 按键 SET 触发 `ai_agent_start` → HTTP `start`，TEN-Agent 创建对应 Graph 处理 RTC 媒体；MUTE 调 `ai_agent_stop` 终止。  
5) 主循环每 10 s `ai_agent_ping` 保活，音视频线程持续发送媒体。

## 扩展与二次开发提示
- 交互层：TEN-Agent 请求体构造集中在 `ai_agent.c`，可在 `properties.v2v` 填入自定义提示词或模型参数，或调整 `graph_name` 选择不同业务流。
- 编解码：`common.h` 内的采样率/帧长/Codec 宏控制上行 PCM 和 RTC payload type，修改需与 TEN-Agent Graph 编解码配置同步；视频在 `video_proc.c` 调整分辨率、质量、子采样以适配带宽。
- 控制入口：按键回调在 `llm_main.c`，若需替换为串口/屏幕 UI/网络命令，可复用 `ai_agent_start/stop/ping` 与音量接口。
- 调试：查看串口日志的入会事件、错误码（`rtc_proc.c`），HTTP 响应打印 code/msg，内存占用每 10 s 打印一次。

## 相关路径速览
- 项目根：`README.md` / `README.cn.md`  
- 主程序入口：`main/llm_main.c`  
- TEN-Agent HTTP：`main/ai_agent.c`  
- RTC 适配：`main/rtc_proc.c`  
- 音频链路：`main/audio_proc.c`  
- 视频链路：`main/video_proc.c`  
- 配置：`main/app_config.h`，`main/Kconfig.projbuild`
