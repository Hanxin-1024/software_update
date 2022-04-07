#ifndef _LOG_H_
#define _LOG_H_
#include <syslog.h>
#include <stdio.h>

#define DEBUG
#define TO_TERMINAL

#ifdef DEBUG
#define LOG_LEVEL_CLSUN   4
#endif

#ifdef RELEASE
#define LOG_LEVEL_CLSUN   3
#endif

#ifndef TO_TERMINAL
#if (LOG_LEVEL_CLSUN > 3)
#define LOGD(format, ...)   syslog(LOG_DEBUG,"LOG_DEBUG-->"format, ##__VA_ARGS__)
#else
#define LOGD(format, ...)
#endif

#if (LOG_LEVEL_CLSUN > 2)
#define LOGI(format, ...)   syslog(LOG_INFO, "LOG_INFO -->"format, ##__VA_ARGS__)
#else
#define LOGI(format, ...)
#endif

#if (LOG_LEVEL_CLSUN > 1)
#define LOGE(format, ...)   syslog(LOG_ERR,  "LOG_ERR  -->"format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)
#endif

#if (LOG_LEVEL_CLSUN > 0)
#define LOG(format, ...)    syslog(LOG_ERR,  format, ##__VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#endif

#ifdef TO_TERMINAL
#if (LOG_LEVEL_CLSUN > 3)
#define LOGD(format, ...)   printf("LOG_DEBUG-->"format, ##__VA_ARGS__)
#else
#define LOGD(format, ...)
#endif

#if (LOG_LEVEL_CLSUN > 2)
#define LOGI(format, ...)   printf("LOG_INFO -->"format, ##__VA_ARGS__)
#else
#define LOGI(format, ...)
#endif

#if (LOG_LEVEL_CLSUN > 1)
#define LOGE(format, ...)   printf("LOG_ERR  -->"format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)
#endif

#if (LOG_LEVEL_CLSUN > 0)
#define LOG(format, ...)   printf(format, ##__VA_ARGS__)
#else
#define LOG(format, ...)
#endif
#endif

#endif

/*
#define LOG_TAG  "MAIN.C  "
#define LOGXX(format, ...) printf("LOG_DEBUG------  "format, ##__VA_ARGS__)
*/