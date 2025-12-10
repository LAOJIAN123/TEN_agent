#pragma once


//LLM Agent Service
// #define TENAI_AGENT_URL       "http://<ip_address>:<port>"
#define TENAI_AGENT_URL       "http://8.134.112.73:3101"
// LLM Agent Graph, you can select openai or gemini 
// #define CONFIG_GRAPH_OPENAI   /* openai, just only audio */
#define CONFIG_GRAPH_GEMINI     /* gemini, for video and audio, but not support chinese language */

/* greeting */
#define GREETING               "Can I help You?"
#define PROMPT                 ""

/* different settings for different agent graph */
#if defined(CONFIG_GRAPH_OPENAI)
#define GRAPH_NAME             "va_openai_v2v"

#define V2V_MODEL              "gpt-realtime"
#define LANGUAGE               "en-US"
#define VOICE                  "ash"
#elif defined(CONFIG_GRAPH_GEMINI)
#define GRAPH_NAME             "doubao_qwen_assistant"
#else
#error "not config graph for aiAgent"
#endif

// LLM Agent Task Name
#define AI_AGENT_NAME          "esp32_client"
// LLM Agent Channel Name
#define AI_AGENT_CHANNEL_NAME  "test"
// LLM User Id
#define AI_AGENT_USER_ID        1001 // user id, for device



/* function config */
/* audio codec：统一改用 G711U，便于与服务器对齐且避免 Opus 崩溃 */
#define CONFIG_USE_OPUS_CODEC
// #define CONFIG_USE_OPUS_CODEC
/* video process */
#define CONFIG_AUDIO_ONLY
