利用ucas开发的刷机工具刷机：
旧刷新  boot.img   偏移0x700000
        kernel.img 偏移0x1300000
        
新刷新  boot.img   偏移0x700000
        kernel.img 偏移0x0
        
假设Z7板子的ip是192.168.1.100，filesize需要根据实际大小填写，offset偏移需要根据"旧刷新"或者是"新刷新"确定

./modify_img kernel.img 0 filesize offset  生成kernel.img_modify   (x86 64位linux系统)

./modify_img boot.img 0 filesize 0x700000  生成boot.img_modify     (x86 64位linux系统)

./client 192.168.1.100 12288 kernel.img_modify   (x86 64位linux系统)

./client 192.168.1.100 12288 boot.img_modify     (x86 64位linux系统)
