#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "log.h"
#include "types.h"
#include "tcp_udp_lib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
 #include <unistd.h>
extern char **environ;

#define DBG_LOG LOGD("%s,%s,line %d\r\n",__FILE__,__func__,__LINE__)
#define MAX_READ_LENGTH 4096
#define RETRY_TIMES 7
#define MAIN_CTL_BOARD_IP    "10.0.1.200"
#define LOG_TAG  "client  :"
#define MEGER_BYTE (1024 * 1024)
#define FILE_PATH  "./kernel.img"
#define MEGA_BYTE  (1024 * 1024)

typedef struct
{
    char data_buff[MAX_READ_LENGTH];
    int data_len;
    int status;
    CHECK_TYPE check_rst;
} payload_content;

static int cfd = -1, fd_file = -1, trans_finished = 1;
static long int count_data_len = 0;
static long int total_file_size = 0;

static void *monitor_func(void *arg)
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
            LOG("snd_data_len = %ld,speed = %.3f MB/s,-->%.0f%%<--\r\n", count_data_len, speed, 100.0 * count_data_len / total_file_size);
        }
        else
        {
            break;
        }
    }
}

static int net_filter(void *inner_arg, void *outer_arg)
{
    (void *)inner_arg;
    (void *)outer_arg;

    LOGD("enter net_filter!\r\n");
    return 1;
}

char* local_itoa(int num,char* str,int radix)
{
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
    unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
    int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
    //获取要转换的整数的绝对值
    if(radix==10&&num<0)//要转换成十进制数并且是负数
    {
        unum=(unsigned)-num;//将num的绝对值赋给unum
        str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
    }
    else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
    //转换部分，注意转换后是逆序的
    do
    {
        str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
        unum/=radix;//unum去掉最后一位
 
    }while(unum);//直至unum为0退出循环
 
    str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
    //将顺序调整过来
    if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
    else k=0;//不是负数，全部都要调整
 
    char temp;//临时变量，交换两个值时用到
    for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
    {
        temp=str[j];//头部赋值给临时变量
        str[j]=str[i-1+k-j];//尾部赋值给头部
        str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
    }
 
    return str;//返回转换后的字符串
 
}

int main(int argc, char **argv)
{
    int i = 0, ret_val = -1;
    int count = 3, retry_count = 0;
    int sin_size = 0;
    unsigned int buffer_len = 1024;
    payload_content *payload = NULL;
    net_load net_data;
    net_reply net_rep;
    CHECK_TYPE check_rst = 0;
    pthread_t monitor_thread;
    struct sockaddr_in s_add, c_add;
    unsigned short portnum = 12288;
    const char *boot_img_name = "BOOT.bin_modify";
    const char *src_boot_img_name = "BOOT.bin";
    int img_file_size = 0;
    unsigned char size_array[15];
    int src_img_fd = -1;
    pid_t fpid = 1;
    int status = 0;
    if (argc < 2)
    {
        LOG("usage: %s server_ip\r\n",argv[0]);
        return 0;
    }
///get file size
    src_img_fd = open(src_boot_img_name,O_RDONLY);
    if (src_img_fd < 0)
    {
        LOGE("open %s failed,ret_val = %d\r\n",src_boot_img_name,errno);
        return -errno;
    }
    img_file_size = lseek(src_img_fd,0,SEEK_END);
    if(img_file_size < 0)
    {
        LOGE("lseek %s failed,ret_val = %d\r\n",src_boot_img_name,errno);
        close(src_img_fd);
        return -errno;
    }
    close(src_img_fd);
    memset(size_array,'\0',sizeof(size_array));
    local_itoa(img_file_size,size_array,10);
    LOGD("img_file_size = %s\r\n",size_array);
    if (fork() == 0)
    {
        fpid = getpid();
        LOGD("child pid = %d\r\n",fpid);
        execl("./modify_img","./modify_img","BOOT.bin","0",size_array,"0",NULL);
        return 0;
        //./modify_img boot.img 0 filesize 0x700000
    }
    else
    {
        waitpid(-1,&status,0);
    }
    payload = malloc(sizeof(payload_content));
    if (payload == NULL)
    {
        LOGE(LOG_TAG"malloc failed,errno = %d\r\n", errno);
        ret_val = -errno;
        goto err_out;
    }

    memset(&net_data, '\0', sizeof(net_load));
    memset(&net_rep, '\0', sizeof(net_reply));

    LOGD(LOG_TAG"sizeof(net_load)  :%ld\r\n", sizeof(net_load));
    LOGD(LOG_TAG"sizeof(net_reply)  :%ld\r\n", sizeof(net_reply));
    LOGD(LOG_TAG"sizeof(payload_content)  :%ld\r\n", sizeof(payload_content));
    LOGD(LOG_TAG"file name :%s\r\n", boot_img_name);
    if (access(boot_img_name, F_OK))
    {
        LOGE(LOG_TAG"file %s does not exist,exit!\r\n,errno = %d", boot_img_name, errno);
        ret_val = -errno;
        goto err_out;
    }

    fd_file = open(boot_img_name, O_RDWR);
    if (fd_file < 0)
    {
        LOGE(LOG_TAG"open %s failed,errno = %d\r\n", boot_img_name, errno);
        return -errno;
    }

    ret_val = lseek(fd_file, 0, SEEK_SET);
    if (ret_val < 0)
    {
        LOGE(LOG_TAG"lseek %s failed,errno = %d\r\n", boot_img_name, errno);
        return -errno;
    }

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == cfd)
    {
        LOGE(LOG_TAG"socket fail ! \r\n");
        return -1;
    }
    LOGD(LOG_TAG"socket ok !\r\n");

    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    if (argc > 1)    s_add.sin_addr.s_addr = inet_addr(argv[1]);
    else            s_add.sin_addr.s_addr = inet_addr(MAIN_CTL_BOARD_IP);
    argv[2] ? portnum = atoi(argv[2]) : 0 ;
    s_add.sin_port = htons(portnum);
    LOGD(LOG_TAG"server ip :%s,portnum :%d\r\n", argv[1], portnum);

    retry_count = 0;
connect_retry:
    if (-1 == connect(cfd, (struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
    {
        if (retry_count++ < RETRY_TIMES)
        {
            for (i = 0; i < retry_count; i++)    usleep(5000000);
            goto connect_retry;
        }
        LOGE(LOG_TAG"connect fail !\r\n");
        return -1;
    }
    LOGD(LOG_TAG"connect ok !\r\n");

    ret_val = read(fd_file, &(net_data.file_head), sizeof(net_data.file_head));
    if (ret_val < 0)
    {
        LOGE("read net_data faild,errno = %d\r\n", errno);
        goto err_out;
    }

    if (net_data.file_head.modify_flag != FILE_MODIFY_FLAG)
    {
        LOGE(LOG_TAG"file is not modifed,check!\r\n");
        goto err_out;
    }

    total_file_size = net_data.file_head.filesize;
    LOGD(LOG_TAG"file size :0x%lx(%ld)\r\n", total_file_size, total_file_size);

    net_data.package_size = MAX_READ_LENGTH;
    net_data.check_rst = crc_32((const unsigned char *)(&net_data), sizeof(net_load) - sizeof(CHECK_TYPE));

    ret_val = write(cfd, &net_data, sizeof(net_load));
    if (ret_val < 0)
    {
        LOGE("write net_data faild,errno = %d\r\n", errno);
        goto err_out;
    }

    ret_val = read(cfd, &net_rep, sizeof(net_reply));
    if (ret_val < 0)
    {
        LOGE("read net_data faild,errno = %d\r\n", errno);
        goto err_out;
    }

    check_rst = crc_32((const unsigned char *)(&net_rep), sizeof(net_reply) - sizeof(CHECK_TYPE));
    LOGD(LOG_TAG"rcv check_rst:0x%x,calcu check_rst:0x%x\r\n", net_rep.check_rst, check_rst);

    if (check_rst != net_rep.check_rst)
    {
        LOGE(LOG_TAG"check_rst erro,rcv check_rst:0x%x,calcu check_rst:0x%x\r\n", net_rep.check_rst, check_rst);
        goto err_out;
    }
    if (net_rep.status != STATUS_OK)
    {
        LOGE(LOG_TAG"ret status(%d) is not status ok ,exit\r\n", net_rep.status);
        goto err_out;
    }

    ret_val = pthread_create(&monitor_thread, NULL, monitor_func, NULL);
    if (ret_val != 0)
    {
        LOGE(LOG_TAG"pthread_create faild for monitor_thread!\r\n");
        return -errno;
    }

    trans_finished = 0;
    while (1)
    {
        ret_val = read(fd_file, payload, MAX_READ_LENGTH);
        if (ret_val < 0)
        {
            LOGE("read payload faild,errno = %d\r\n", errno);
            payload->data_len = 0;
            payload->status = -1;
        }
        else
        {
            count_data_len += ret_val;
            payload->data_len = ret_val;
            if (ret_val == MAX_READ_LENGTH) payload->status = 0;
            else                            payload->status = 1;
        }

        payload->check_rst = crc_32((const unsigned char *)payload, sizeof(payload_content) - sizeof(CHECK_TYPE));

        //ret_val = write(cfd,payload,sizeof(payload_content));
        ret_val = tcp_udp_write(0, cfd, (char *)payload, sizeof(payload_content));
        if (ret_val < 0)
        {
            LOGE("write payload faild,errno = %d\r\n", errno);
            break;
        }
        else if (ret_val < sizeof(payload_content))
        {
            LOGE(LOG_TAG"tcp_udp_write retry overflow!\r\n");
        }

        //ret_val = read(cfd,&net_rep,sizeof(net_reply));
        ret_val = tcp_udp_read(0, cfd, (char *)(&net_rep), sizeof(net_reply), NULL, NULL);
        if (ret_val < 0)
        {
            LOGE("read payload faild,errno = %d\r\n", errno);
            break;
        }
        else if (ret_val < sizeof(net_reply))
        {
            LOGE(LOG_TAG"tcp_udp_read retry overflow!\r\n");
        }

        check_rst = crc_32((const unsigned char *)(&net_rep), sizeof(net_reply) - sizeof(CHECK_TYPE));
        if (check_rst != net_rep.check_rst)
        {
            LOGE(LOG_TAG"check_rst erro,rcv check_rst:0x%x,calcu check_rst:0x%x\r\n", net_rep.check_rst, check_rst);
            break;
        }

        if (payload->status < 0) break;
        if (net_rep.status != STATUS_OK)
        {
            break;
        }
    }
    sleep(3);
    LOGI("total len:%ld\r\n", count_data_len);
    trans_finished = 1;
    //wait for spi flash handle done!
    LOG("waiting for writing spi flash done...\r\n");
    ret_val = tcp_udp_read(0, cfd, (char *)(&net_rep), sizeof(net_reply), NULL, NULL);
    if (ret_val < 0)
    {
        LOGE("read payload faild,errno = %d\r\n", errno);
        goto wait_done_err_out;
    }
    else if (ret_val < sizeof(net_reply))
    {
        LOGE(LOG_TAG"tcp_udp_read retry overflow!\r\n");
    }

    check_rst = crc_32((const unsigned char *)(&net_rep), sizeof(net_reply) - sizeof(CHECK_TYPE));
    if (check_rst != net_rep.check_rst)
    {
        LOGE(LOG_TAG"check_rst erro,rcv check_rst:0x%x,calcu check_rst:0x%x\r\n", net_rep.check_rst, check_rst);
        goto wait_done_err_out;
    }

    if (net_rep.status != STATUS_OK)
    {
        LOGE(LOG_TAG"writing spi flash err!\r\n");
    }
    else
    {
        LOG("writing spi flash done!\r\n");
    }

wait_done_err_out:
    pthread_join(monitor_thread, NULL);
err_out:
    if (cfd > 0) close(cfd);
    if (payload) free(payload);
    if (fd_file > 0) close(fd_file);
    return ret_val;
}
