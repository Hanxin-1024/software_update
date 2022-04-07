#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <pthread.h>
#include "log.h"
#include "tcp_udp_lib.h"
#include "types.h"
#include <arpa/inet.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

typedef int STATUS_TYPE;
typedef int DATA_LEN_TYPE;

#define get_data_len_ptr(prt,size)    ((DATA_LEN_TYPE *)(&(prt)[(size) - sizeof(DATA_LEN_TYPE) - sizeof(STATUS_TYPE) - sizeof(CHECK_TYPE)]))
#define get_status_ptr(prt,size)    ((STATUS_TYPE *)(&(prt)[(size) - sizeof(STATUS_TYPE) - sizeof(CHECK_TYPE)]))
#define get_check_rst_ptr(prt,size) ((CHECK_TYPE *)(&((prt)[(size) - sizeof(CHECK_TYPE)])))
#define DBG_LOG LOGD("%s,%s,line %d\r\n",__FILE__,__func__,__LINE__)
#define LOG_TAG "server  :"
#define MEGA_BYTE (1024 * 1024)

#define SET_UPDATE_FLAG      "mtd_debug write /dev/mtd3 0x500002 0x40000 /dev/zero 2> /dev/null 1> /dev/null"   //oxffff0000 means the flash is rewrited
#define CLEAR_UPDATE_FLAG    "mtd_debug erase /dev/mtd3 0x500000 0x40000 2> /dev/null 1> /dev/null"             //oxffffffff means the flash is not rewrited
typedef struct
{
    file_type file_t;
    const char *file_name;
    const char *mtd_name;
} flash_block_info;

static flash_block_info flash_block[] =
{
    {ENUM_BOOT, "BOOT.bin", "/dev/mtd0"},
    {ENUM_BOOTENV, "bootenv.img", "/dev/mtd1"},
    {ENUM_KERNEL, "kernel.img", "/dev/mtd2"},
};
static int connect_count = 0;
static int trans_finished = 1;

static long int count_data_len = 0;
static long int total_file_size = 0;
void *monitor_func(void *arg)
{
    float speed = 0;
    int delay_s = 2;
    long int count_data_len_old = 0;
    while (1)
    {
        sleep(delay_s);
        if (!trans_finished)
        {
            speed = (count_data_len - count_data_len_old) * 1.0 / MEGA_BYTE / delay_s;
            count_data_len_old = count_data_len;
            LOG("rcv_data_len = %ld,speed = %.3f MB/s,-->%.0f%%<--\r\n", count_data_len, speed, 100.0 * count_data_len / total_file_size);
        }
        else
        {
            count_data_len_old = 0;
            count_data_len = 0;
        }
    }
}

static int open_and_truncate(const char *pathname)
{
    int ret_val = -1, fd_file = -1, file_flag = O_RDWR;

    if (access(pathname, F_OK))
    {
        LOGE(LOG_TAG"create file :%s\r\n", pathname);
        file_flag |= O_CREAT;
    }

    fd_file = open(pathname, file_flag);
    if (fd_file < 0)
    {
        LOGI(LOG_TAG"open %s failed,errno = %d\r\n", pathname, errno);
        return -errno;
    }
    ret_val = ftruncate(fd_file, 0);
    if (ret_val < 0)
    {
        LOGE(LOG_TAG"ftruncate %s failed,errno = %d\r\n", pathname, errno);
        return -errno;
    }
    ret_val = lseek(fd_file, 0, SEEK_SET);
    if (ret_val < 0)
    {
        LOGE(LOG_TAG"lseek %s failed,errno = %d\r\n", pathname, errno);
        return -errno;
    }

    chmod(pathname, 0766);

    return fd_file;
}

static int net_filter(void *inner_arg, void *outer_arg)
{
    (void *)inner_arg;
    (void *)outer_arg;

    LOGD("enter net_filter!\r\n");
    return 1;
}

#define BAK_FILE_NAME "tmp_boot_bin.bak"
static void *get_data_func(void *arg)
{
    int ret_val = -1, file_fd = -1, payload_size = 0;
    unsigned int i = 0;
    int update_succeed = 0;
    int tmp_nfp = *((int *)arg);
    net_load net_data;
    net_reply net_rep;
    CHECK_TYPE check_rst = 0;
    unsigned char *payload = NULL;
    char cmd_buf[100];

    memset(&net_data, '\0', sizeof(net_load));
    memset(&net_rep, '\0', sizeof(net_reply));
    connect_count++;
    LOGD("enter %s func\r\n", __func__);
    //get fileinfo
    net_rep.status = STATUS_OK;
    ret_val = tcp_udp_read(0, tmp_nfp, (char *)(&net_data), sizeof(net_load), NULL, NULL);
    if (ret_val < 0)
    {
        LOGE("get net_data faild,errno = %d\r\n", errno);
        goto get_data_err_out;
    }
    else if (ret_val < sizeof(net_load))
    {
        LOGE(LOG_TAG"tcp_udp_read retry overflow!\r\n");
    }

    check_rst = crc_32((const unsigned char *)(&net_data), sizeof(net_load) - sizeof(CHECK_TYPE));
    if (check_rst != net_data.check_rst)
    {
        LOGE(LOG_TAG"ret_val = %d,check_rst erro,rcv check_rst:0x%x,calcu check_rst:0x%x\r\n", ret_val, net_data.check_rst, check_rst);
        net_rep.status = STATUS_CHECK_ERR;
    }

    net_rep.check_rst = crc_32((const unsigned char *)(&net_rep), sizeof(net_reply) - sizeof(CHECK_TYPE));
    ret_val = write(tmp_nfp, &net_rep, sizeof(net_reply));
    if (ret_val < 0)
    {
        LOGE("write net_data faild,errno = %d\r\n", errno);
        goto get_data_err_out;
    }

    if (net_rep.status != STATUS_OK)
    {
        goto get_data_err_out;
    }

    LOGD(LOG_TAG"file_type :%d,position :0x%x,filesize :0x%x(%d),offset :0x%x,package_size :%d\r\n", \
         net_data.file_head.file_t, net_data.file_head.position, net_data.file_head.filesize, net_data.file_head.filesize, net_data.file_head.offset, net_data.package_size);

    total_file_size = net_data.file_head.filesize;//get file size

    file_fd = open_and_truncate(BAK_FILE_NAME);
    if (file_fd < 0)
    {
        LOGE("open_and_truncate faild,errno = %d\r\n", errno);
        goto get_data_err_out;
    }

    payload_size = (net_data.package_size % 4 == 0 ? net_data.package_size : 4 * (net_data.package_size / 4 + 1)) + sizeof(DATA_LEN_TYPE) + sizeof(STATUS_TYPE) + sizeof(CHECK_TYPE);//payload,data_len(int),status(int),check_rst(int)
    payload = (char *)malloc(payload_size);
    if (payload == NULL)
    {
        LOGE(LOG_TAG"malloc failed,errno = %d\r\n", errno);
        goto get_data_err_out;
    }
    memset(payload, '\0', payload_size);
    LOGD(LOG_TAG"payload_size = %d\r\n", payload_size);
    //get file content
    while (1)
    {
        net_rep.status = STATUS_OK;
        ret_val = tcp_udp_read(0, tmp_nfp, payload, payload_size, NULL, NULL);
        if (ret_val < 0)
        {
            LOGE("get net_data faild,errno = %d\r\n", errno);
            goto get_data_err_out;
        }
        else if (ret_val < payload_size)
        {
            LOGE(LOG_TAG"tcp_udp_read retry overflow!\r\n");
        }

        check_rst = crc_32(payload, payload_size - sizeof(CHECK_TYPE));
        if (check_rst != *get_check_rst_ptr(payload, payload_size))
        {
            LOGE(LOG_TAG"check_rst erro,rcv check_rst:0x%x,calcu check_rst:0x%x\r\n", *get_check_rst_ptr(payload, payload_size), check_rst);
            net_rep.status = STATUS_CHECK_ERR;
            LOGD("ret_val = %d\r\n", ret_val);
            for (i = 0; i < ret_val; i++)
            {
                i % 64 == 0 ? LOG("\r\n") : 0;
                LOG("%02x ", payload[i]);
            }
            LOG("\r\n");
            goto resp_ack;
        }
        count_data_len += *get_data_len_ptr(payload, payload_size);

        ret_val = write(file_fd, payload, *get_data_len_ptr(payload, payload_size));
        if (ret_val < 0)
        {
            LOGE("write %s faild,errno = %d\r\n", BAK_FILE_NAME, errno);
            net_rep.status = STATUS_WRITE_ERR;
            goto resp_ack;
        }

        if (*get_status_ptr(payload, payload_size) == 0) // 0-->ok,>0-->done,<0-->err
        {
            net_rep.status = STATUS_OK;
        }
        else if (*get_status_ptr(payload, payload_size) > 0)
        {
            net_rep.status = STATUS_FINISHED;
        }
        else if (*get_status_ptr(payload, payload_size) < 0)
        {
            net_rep.status = STATUS_READ_ERR;
        }
resp_ack:
        net_rep.check_rst = crc_32((const unsigned char *)(&net_rep), sizeof(net_reply) - sizeof(CHECK_TYPE));
        ret_val = tcp_udp_write(0, tmp_nfp, (char *)(&net_rep), sizeof(net_reply));
        if (ret_val < 0)
        {
            LOGE("write net_data faild,errno = %d\r\n", errno);
            goto get_data_err_out;
        }
        else if (ret_val < sizeof(net_reply))
        {
            LOGE(LOG_TAG"tcp_udp_write retry overflow!\r\n");
        }

        if (net_rep.status != STATUS_OK)
        {
            if (net_rep.status == STATUS_FINISHED)      break;
            else                                        goto get_data_err_out;
        }
    }
    sleep(3);
    trans_finished = 1;
    LOG(LOG_TAG"data trans finished,rcv %ld(Bytes) data\r\n", count_data_len);
    LOG(LOG_TAG"start to update ");

    net_rep.status = STATUS_OK;
    for (i = 0; i < ARRAY_SIZE(flash_block); i++)
    {
        if (net_data.file_head.file_t == flash_block[i].file_t)
        {
            LOG("%s\r\n", flash_block[i].file_name);
            //clear flag bit
            // ret_val = system(CLEAR_UPDATE_FLAG);
            // if (ret_val < 0)
            // {
            //     LOGE("system(3) CLEAR_UPDATE_FLAG faild,errno = %d\r\n", errno);
            //     goto get_data_err_out;
            // }

            // memset(&cmd_buf, '\0', sizeof(cmd_buf));
            // snprintf(cmd_buf, sizeof(cmd_buf), "%s %s 0x%x 0x%x %s_bak", "mtd_debug read", flash_block[i].mtd_name, net_data.file_head.offset, 0x1400000, flash_block[i].file_name);
            // LOG(LOG_TAG"make a backup...\r\n");
            // LOG(LOG_TAG"%s\r\n", cmd_buf);
            // ret_val = system(cmd_buf);
            // if (ret_val < 0)
            // {
            //     net_rep.status = STATUS_READ_ERR;
            //     LOGE("system(3) faild,errno = %d\r\n", errno);
            //     goto get_data_err_out;
            // }
            //LOG(LOG_TAG"make backup done!\r\n");

            memset(&cmd_buf, '\0', sizeof(cmd_buf));
            snprintf(cmd_buf, sizeof(cmd_buf), "%s %s 0x%x 0x%x", "mtd_debug erase", flash_block[i].mtd_name, net_data.file_head.offset, 0x100000 * (net_data.file_head.filesize / 0x100000 + (net_data.file_head.filesize % 0x100000 ? 1 : 0)));
            LOG(LOG_TAG"erase flash...\r\n");
            LOG(LOG_TAG"%s\r\n", cmd_buf);
            ret_val = system(cmd_buf);
            if (ret_val < 0)
            {
                net_rep.status = STATUS_WRITE_ERR;
                LOGE("system(3) faild,errno = %d\r\n", errno);
                goto update_err_out;
            }
            LOG(LOG_TAG"erase flash done!\r\n");

            memset(&cmd_buf, '\0', sizeof(cmd_buf));
            snprintf(cmd_buf, sizeof(cmd_buf), "%s %s 0x%x 0x%x %s", "mtd_debug write", flash_block[i].mtd_name, net_data.file_head.offset, net_data.file_head.filesize, BAK_FILE_NAME);
            LOG(LOG_TAG"write flash...\r\n");
            LOG(LOG_TAG"%s\r\n", cmd_buf);
            ret_val = system(cmd_buf);
            if (ret_val < 0)
            {
                net_rep.status = STATUS_WRITE_ERR;
                LOGE("system(3) faild,errno = %d\r\n", errno);
                goto update_err_out;
            }
            update_succeed = 1;
            //set flag bit
            // ret_val = system(SET_UPDATE_FLAG);
            // if (ret_val < 0)
            // {
            //     LOGE("write flash done,but something goes wrong when set flag bit in spi_flash,errno = %d\r\n", errno);
            // }

            LOG(LOG_TAG"write flash done!\r\n");
update_err_out:
            // memset(&cmd_buf, '\0', sizeof(cmd_buf));
            // snprintf(cmd_buf, sizeof(cmd_buf), "%s_bak", flash_block[i].file_name);
            // LOG(LOG_TAG"remove file:%s\r\n", cmd_buf);
            // remove(cmd_buf);

            break;
        }
    }

    if (i == ARRAY_SIZE(flash_block))
    {
        net_rep.status = STATUS_WRITE_ERR;
        LOGE(LOG_TAG"parameter gose wrong,exit");
    }

get_data_err_out:
    trans_finished = 1;
    sleep(1);
    net_rep.check_rst = crc_32((const unsigned char *)(&net_rep), sizeof(net_reply) - sizeof(CHECK_TYPE));
    ret_val = tcp_udp_write(0, tmp_nfp, (char *)(&net_rep), sizeof(net_reply));
    if (ret_val < 0)
    {
        LOGE("write net_data faild,errno = %d\r\n", errno);
    }
    else if (ret_val < sizeof(net_reply))
    {
        LOGE(LOG_TAG"tcp_udp_write retry overflow!");
    }

    if (payload) free(payload);
    connect_count--;
    if (file_fd > 0)
    {
        close(file_fd);
        //remove(BAK_FILE_NAME);
    }
    close(tmp_nfp);

    if (update_succeed)
    {
        LOG(LOG_TAG"update flash succeed!\r\n");
    }
}

#define MAX_LINK_NUM 5
int main(int argc, char **argv)
{
    int ret_val = -1, i = 0, server_fd = -1, server_nfp = -1;
    struct sockaddr c_add;
    unsigned int portnum = 12288;
    pthread_t test_thread;
    pthread_t monitor_thread;

    argv[1] ?  portnum = atoi(argv[1]) : 0;
    LOGD(LOG_TAG"portnum = %d\r\n", portnum);

    server_fd = tcp_udp_server_init(0, portnum, MAX_LINK_NUM);
    if (server_fd < 0)
    {
        LOGE(LOG_TAG"tcp socket failed,errno = %d\r\n", errno);
        return -errno;
    }
    LOGD(LOG_TAG"server_fd = %d\r\n", server_fd);

    ret_val = pthread_create(&monitor_thread, NULL, monitor_func, NULL);
    if (ret_val != 0)
    {
        LOGE(LOG_TAG"pthread_create faild for monitor_thread!\r\n");
        return -errno;
    }

    while (1)
    {
        server_nfp = tcp_udp_accept(0, server_fd, &c_add);
        LOGD("tcp_udp_accept ok \r\n");
        trans_finished = 0;
        if (connect_count >= MAX_LINK_NUM)
        {
            if (server_nfp >= 0)    close(server_nfp);
            server_nfp = -1;
            LOGE(LOG_TAG"link num overflow,close the latest bound!\r\n");
            continue;
        }

        ret_val = pthread_create(&test_thread, NULL, get_data_func, (void *)(&server_nfp));
        if (ret_val != 0)
        {
            LOGE(LOG_TAG"pthread_create faild for test_thread!\r\n");
            return -errno;
        }
    }

    pthread_join(test_thread, NULL);
    pthread_join(monitor_thread, NULL);
    close(server_fd);
    return 0;
}
