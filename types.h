#ifndef _TYPES_H_
#define _TYPES_H_
#include <stdio.h>
#include "crc.h"

#define ARRAY_SIZE(x)   (sizeof((x)) / sizeof((x)[0]))
#define FILE_MODIFY_FLAG 0xdeadbeef
typedef enum
{
    ENUM_BOOT = 1, ENUM_BOOTENV, ENUM_KERNEL,
} file_type;

typedef enum
{
    STATUS_OK = 0, STATUS_READ_ERR, STATUS_WRITE_ERR, STATUS_CHECK_ERR, STATUS_FINISHED,
} ack_type;

typedef struct
{
    unsigned int modify_flag;
    file_type file_t;
    int position;
    int filesize;
    int offset;
} file_head_info;

typedef struct
{
    file_head_info file_head;
    unsigned int package_size;
    CHECK_TYPE check_rst;
} net_load;

typedef struct
{
    ack_type status;
    CHECK_TYPE check_rst;
} net_reply;
/*
file = file_head_info + file_content
*/
#endif

/*
#define LOG_TAG  "MAIN.C  "
#define LOGXX(format, ...) printf("LOG_DEBUG------  "format, ##__VA_ARGS__)
*/