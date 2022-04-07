刷机原理：Z7是服务端，与Z7连接的电脑是客户端，通过tcp将刷机文件发送到Z7以后利用spiflash操作工具进行flash的读写以达到刷机的目的
刷kernel步骤如下(假设Z7板子的ip是192.168.1.100)：
1.z7运行
./onlineupdate 
2.将BOOT.bin拷贝到client_update所在文件夹下
3.客户机设置网卡ip到同一网段，运行
./client_update 192.168.1.100 等待刷机完成
