#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "log.h"
#include "types.h"
#include <string.h>

#define LOG_TAG "modify_img.c :"
#define ARRAY_LEN 3

typedef struct
{
    const char *file_name_part;
    file_type file_t;
    int position;
    int filesize;
    int offset;
} flash_block_info;

static flash_block_info flash_block[ARRAY_LEN] =
{
    {"BOOT", ENUM_BOOT, 1, 1024, 3},
    {"bootenv", ENUM_BOOTENV, 2, 1024, 4},
    {"kernel", ENUM_KERNEL, 3, 1024, 6},
};

int main(int argc, char *argv[])
{
    int ret_val = -1, i = 0, j = 0;
    int fd_file = -1, file_flag = O_RDWR;;
    char cmd_buff[100];
    file_head_info file_head;

    if (argc < 5)
    {
        LOG("usage:  ./modify_img_1 img_name position filesize offset\r\n");
        return -1;
    }
    memset(&file_head, '\0', sizeof(file_head));
    memset(cmd_buff, '\0', sizeof(cmd_buff));

    //LOGD(LOG_TAG"sizeof(net_load) = %ld\r\n",sizeof(net_load));
    //LOGD(LOG_TAG"sizeof(file_head_info) = %ld\r\n",sizeof(file_head_info));

    for (j = 0; j < ARRAY_LEN; j++)
    {
        if (strstr(argv[1], flash_block[j].file_name_part))
        {
            file_head.modify_flag = FILE_MODIFY_FLAG;
            file_head.file_t = flash_block[j].file_t;
            if (argv[2])    file_head.position = strtol(argv[2], NULL, 0);
            else            file_head.position = flash_block[j].position;
            if (argv[3])    file_head.filesize = strtol(argv[3], NULL, 0);
            else            file_head.filesize = flash_block[j].filesize;
            if (argv[4])    file_head.offset   = strtol(argv[4], NULL, 0);
            else            file_head.offset   = flash_block[j].offset;
        }
    }
    LOG("filename : %s,\nposition : 0x%x,\nfilesize : 0x%x(%d),\noffset   : 0x%x\r\n", argv[1], file_head.position, file_head.filesize, file_head.filesize, file_head.offset);

    memset(cmd_buff, '\0', sizeof(cmd_buff));
    snprintf(cmd_buff, sizeof(cmd_buff), "%s_modify", argv[1]);
    if (access(cmd_buff, F_OK))
    {
        LOGI(LOG_TAG"file %s does not exist,create it!\r\n", cmd_buff);
        file_flag |= O_CREAT;
    }
    else
    {
        memset(cmd_buff, '\0', sizeof(cmd_buff));
        snprintf(cmd_buff, sizeof(cmd_buff), "cat /dev/null > %s_modify", argv[1]);
        ret_val = system(cmd_buff);
        if (ret_val < 0)
        {
            LOGE("system(3) faild,errno = %d\r\n", errno);
            return -errno;
        }
    }

    memset(cmd_buff, '\0', sizeof(cmd_buff));
    snprintf(cmd_buff, sizeof(cmd_buff), "%s_modify", argv[1]);
    fd_file = open(cmd_buff, file_flag);
    if (fd_file < 0)
    {
        LOGE(LOG_TAG"open %s failed,errno = %d\r\n", cmd_buff, errno);
        return -errno;
    }
    chmod(cmd_buff, 0766);

    ret_val = write(fd_file, &file_head, sizeof(file_head_info));
    if (fd_file < 0)
    {
        LOGE(LOG_TAG"write %s failed,errno = %d\r\n", cmd_buff, errno);
        if (fd_file > 0)    close(fd_file);
        return -errno;
    }
    if (fd_file > 0)    close(fd_file);

    memset(cmd_buff, '\0', sizeof(cmd_buff));
    snprintf(cmd_buff, sizeof(cmd_buff), "cat %s >> %s_modify", argv[1], argv[1]);
    LOG("cmd_buff :%s\r\n",cmd_buff);
    ret_val = system(cmd_buff);
    if (ret_val < 0)
    {
        LOGE("system(3) faild,errno = %d\r\n", errno);
        return -errno;
    }

    LOG("file %s modify done\r\n", argv[1]);

    return 0;
}