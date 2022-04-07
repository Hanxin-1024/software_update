#!/bin/bash

cd $(cd `dirname $0`; pwd)

export MODE=${MODE:-"Debug"}
export PLATFORM=${PLATFORM:-"x64"}
export REDIRECT=${REDIRECT:-"to_term"}

if [ "$MODE" == "Release" ];then
    CFLAG="RELEASE"
else
    CFLAG="DEBUG"
    MODE="Debug"
fi

if [ "$PLATFORM" == "x86" ];then
    APPABI="-m32"
else
    PLATFORM="x64"
    APPABI=""
fi

if [ "$REDIRECT" == "to_file" ];then
    LOG_DIRECTION="TO_FILE"
else
    REDIRECT="to_term"
    LOG_DIRECTION="TO_TERMINAL"
fi

echo "MODE     : Debug   / Release (default Debug)"
echo "PLATFORM : x64     / x86 (default x64)"
echo "REDIRECT : to_term / to_file (default to_file)"
echo -e "\n"

echo -e "\033[32m--------------------------------------------------------- \033[0m"
echo -e "\033[32m compile begein! \033[0m"
echo "MODE     : $MODE"
echo "PLATFORM : $PLATFORM"
echo "REDIRECT : $REDIRECT"
./version_info/version_gen.sh

if [ $? -eq 0 ]; then
    echo -e "\033[32m version_gen successful! \033[0m"
else
    echo -e "\033[31m -----------version_gen.sh failed!------------- \033[0m"
fi
gcc $APPABI -I ./ -I ./crc -I ./version_info -I ./tcp_udp_lib -o onlineupdate onlineupdate.c ./version_info/version.c ./crc/crc.c ./tcp_udp_lib/tcp_udp_lib.c -lpthread  -D $CFLAG -D $LOG_DIRECTION
gcc $APPABI -I ./ -I ./crc -I ./version_info -I ./tcp_udp_lib -o client client.c ./version_info/version.c ./crc/crc.c ./tcp_udp_lib/tcp_udp_lib.c -lpthread  -D $CFLAG -D $LOG_DIRECTION
gcc $APPABI -I ./ -I ./crc -I ./version_info -I ./tcp_udp_lib -o modify_img modify_img.c ./version_info/version.c ./crc/crc.c ./tcp_udp_lib/tcp_udp_lib.c -lpthread  -D $CFLAG -D $LOG_DIRECTION
if [ $? -eq 0 ]; then
    echo -e "\033[32m Successfully build! \033[0m"
else
    echo -e "\033[31m -----------build failed!------------- \033[0m"
fi
echo -e "\033[32m--------------------------------------------------------- \033[0m"
