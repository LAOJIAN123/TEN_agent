# TEN Framework AI Agent Docker 启动指南（草稿）



#ssh命令
ssh jian@192.168.2.51
password:Linzhijian666

> 目标：在本地用 Docker 快速拉起 `ai_agents` 示例，跑通后再按需定制。后续可继续补充细节。

## 前置准备
- 安装 Docker 与 Docker Compose（Windows/macOS 可用 Docker Desktop）。
- 准备必需的 API Key：Agora `APP_ID` 与 `APP_CERTIFICATE`，至少一个 LLM（如 OpenAI）、一个 ASR（如 Deepgram）、一个 TTS（如 ElevenLabs）。
- 推荐：Node.js LTS 18+（便于本地调试/构建前端）。

## 拉取仓库并配置环境
```bash
# 克隆项目后进入 ai_agents 子目录
cd ai_agents

# 复制环境变量模板
cp .env.example .env

# 填写 .env 内的密钥（必填：AGORA、LLM、ASR、TTS 等）
```

常用字段示例：
```env
AGORA_APP_ID=
AGORA_APP_CERTIFICATE=
DEEPGRAM_API_KEY=
OPENAI_API_KEY=
OPENAI_MODEL=gpt-4o
ELEVENLABS_TTS_KEY=
```

## 使用 Docker 本地启动
```bash
# 1) 拉起开发容器（后台运行）
docker compose up -d

# 2) 进入容器
docker exec -it ten_agent_dev bash

# 3) 选择示例，进入对应目录
cd agents/examples/voice-assistant          # 串联语音助手
# cd agents/examples/voice-assistant-realtime # 实时对话助手（可选）

# 4) 安装依赖并运行（若改过源码先执行 task build）
#如果有旧的软链/文件残留安装失败可以使用
#cd /app/agents/examples/voice-assistant
#rm -rf node_modules
#bun install --verbose

task install
task run
```
# 如果出现task run失败：缺少.env的情况
# 注意在宿主机文件根目录.env 和ai_agent/.env#开头加上：    #AGORA_APP_ID=59515fae501747c2973b2d47d765f031
# AGORA_APP_CERTIFICATE=6c944086d7964f9fbe1a20d561788b56


启动后访问：
- TMAN Designer：<http://localhost:49483>
- Agent 示例 UI：<http://localhost:3000>

## 常用自定义
1) 在 TMAN Designer（49483）里右键 STT / LLM / TTS 扩展，填入对应 API Key 并提交。
2) 如需修改属性文件，可直接编辑 `agents/examples/<example>/property.json`，然后重新 `task run`。

## 构建发布用 Docker 镜像（可选）
在容器外执行：
```bash
cd ai_agents
# 将 <example-name> 换成实际示例目录，如 voice-assistant
docker build -f agents/examples/<example-name>/Dockerfile -t example-app .

# 运行发布镜像
docker run --rm -it --env-file .env -p 3000:3000 example-app
```

## 常见问题速查
- 端口被占用：修改 `docker-compose.yml` 或运行时的 `-p` 端口映射。
- 密钥相关报错：确认 `.env` 已填且容器内能读取；可在容器内 `cat .env` 校验。
- 国内网络下载依赖慢：在 shell 中配置 HTTP(S) 代理；必要时为 Docker 和 npm/pip/Go 单独设置代理源。
- `.env` 未加载或日志目录创建失败：根因是没有 `.env` 或缺少 `LOG_PATH` 等变量。解决：在仓库根和 `ai_agents/.env` 填写 `LOG_PATH=./logs`、`LOG_STDOUT=true`、`SERVER_PORT=8080`、`WORKERS_MAX=4`、`WORKER_QUIT_TIMEOUT_SECONDS=60` 等，并确保对应目录存在（`mkdir logs` / `mkdir ai_agents/logs`）。
- `Module not found: Can't resolve 'lucide-react'`（前端 3000 端口）：进入对应前端目录安装依赖。示例：`cd ai_agents/agents/examples/rtm-transport/frontend && npm ci`；若锁文件不一致就改用 `npm install` 或删除 `node_modules` 后 `bun install --verbose`。
- 局域网访问 TMAN Designer：compose 默认将容器 49483 映射到宿主机 9501。其他电脑用 `http://<宿主机IP>:9501` 访问；若想直接用 49483，可把映射改成 `"49483:49483"` 后重启 `docker compose up -d`，并确保宿主机防火墙放行端口。

---
本指南为初版草稿，后续可补充更多示例、截图或排障细节。
