
// 蓝牙检测文件
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <string>
#include "bluetooth.h"
#include "usbctl.h"
#include <iostream>


int blue_proc_message(string &meg)
{
    //cout << "meg: " << meg << endl;
    if(meg == "")
    {
        return 1;
    }
    if(strstr(meg.c_str(), "END") != NULL)
    {
        //搜索到了末尾
        return 3;
    }
    else if(strstr(meg.c_str(), "END") == NULL && strstr(meg.c_str(), "g2020") != NULL)
    {
        //没有搜索到末尾，且查询到g2020的蓝牙了
        meg = meg.substr(0, 12);
        return 2;
    }
    else if(strstr(meg.c_str(), "END") == NULL && strstr(meg.c_str(), "g2020") == NULL)
    {
        //没有搜索到末尾，且不是g2020的蓝牙了
        return 1;
    }
}
// 遍历com端口，寻找bluetooth设备并打开
// 采用 select 方式检测超时
// 先发送 AT 指令测试，测试是否启动，再发送 AT+RST 指令进行重启操作，重启成功，完成配对
// @param
// @return 找到并返回文件描述符，若未找到，返回-1
int bluetooth_open(string &device_mac)
{
    int fd, ret;
    char *p;
    char buf[60];
    char in;
    uint8_t RST[] = "AT+Z=1\r\n";             //重启
    uint8_t OPEN_VISUAL[] = "AT+SPP=1\r\n";   //设置可见
    uint8_t AT_SCAN[] = "AT+SCAN?\r\n";     //扫描蓝牙设备
    uint8_t DISCONNECT[] = "AT+DISCON=1\r\n";
    string mac;                             //用于获取路由器蓝牙的mac
    string pro_message = "";
    // select 方式
    struct timeval timeout;
    fd_set fdset;
    FD_ZERO(&fdset);

    // 遍历com口
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int i = 5;
    while(i > 0)
    {
        fd = open_port(2);
        if (-1 == fd)
        {
            i--;
            continue;
        }
        else{
            break;
        }
    }
    if(fd != -1)
    {
        // 配置USB端口
        ret = set_opt(fd, 115200, 8, 'n', 1);
        FD_SET(fd,&fdset);
        // 开始测试 bluetooth设备 AT指令
        // 检测设备启动
        system("/home/root/jd_open.sh");//拉低switch管脚电平，进入AT命令模式
        sleep(1);
        write_port(fd, DISCONNECT, strlen((const char*)DISCONNECT));
        system("/home/root/jd_close.sh");   //拉高switch管脚电平
        write_port(fd, (const char*)RST, strlen((const char*)RST));
        sleep(1);
        // 监听事件 1秒钟 超时跳过
        ret = select(fd + 1, &fdset, NULL, NULL, &timeout);
        switch (ret)
        {
        case -1:                                  //错误
            printf("->> Error occurs when select() \n");
            break;
        case 0:
            printf("->> Select() timeout \n");        //超时
            break;
        default:
            if (FD_ISSET(fd, &fdset))
            {
                ret = read_port(fd, buf, 60);
                if (ret > 0)
                {
                     // 检查AT指令返回值
					p = strstr(buf, "OK");
                    if (p != NULL)
                    {
                        //设备重启成功
                        ret = write_port(fd, OPEN_VISUAL, strlen((const char*)OPEN_VISUAL));
                        sleep(1);
                        ret = write_port(fd, AT_SCAN, strlen((const char*)AT_SCAN));
                        //开始扫描
                        while(1)
                        {
                            FD_ZERO(&fdset);
                            FD_SET(fd, &fdset);
                            ret = select(fd + 1, &fdset, NULL, NULL, NULL);
                            if(ret > 0)
                            {
                                if(read(fd, &in, 1) == 0)continue;
                                if(in != '\n')
                                {
                                    //判断不是换行符
                                   pro_message = pro_message + in; 
                                }
                                else
                                {
                                    //一行末尾调用process_message函数
                                    //cout << "pro_message: " << pro_message << endl;
                                    int ret_proc_mess = blue_proc_message(pro_message);
                                    if(ret_proc_mess == 1)
                                    {
                                        //返回值为1表示还没有到末尾，且没有查询到
                                        pro_message = "";
                                        continue;
                                    }
                                    else if(ret_proc_mess == 2)
                                    {
                                        //返回值为2表示还没有到末尾，且查询到了
                                        device_mac = pro_message;
                                        cout << "g2020: " << device_mac << endl;
                                        return fd;
                                    }
                                    else if(ret_proc_mess == 3)
                                    {
                                        //查询到了末尾，且没有查询到，则退出
                                        FD_CLR(fd, &fdset);
                                        close(fd);
                                        return -1;
                                    }
                                }
                                //监听到连接
                                /*ret = read_port(fd, buf, 60);
                                if(strstr(buf, "END") != NULL)
                                {
                                    //证明扫描到末尾了,但是还没有找到
                                    return -1;
                                }
                                else
                                {
                                    //没有扫到末尾
                                    string temp_scan(buf, buf + ret);
                                    cout << temp_scan << endl;
                                    if(strstr(buf, "g2020") != NULL)
                                    {
                                        //扫到g2020这个蓝牙了
                                        string mac(buf, buf + 11);
                                        device_mac = mac;
                                        return fd;
                                    } 
                                }*/ 
                            }
                        }
                    }
                }
            }
        }
    }
    FD_CLR(fd, &fdset);
    close(fd);
    return -1;
}

int getbtinfo()
{
}

int getbtsta()
{
}
