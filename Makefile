CC = arm-linux-gnueabihf-gcc
APP = onlineupdate
CLIENT_APP = client_update
MODIFY_APP = modify_img

# Add any other object files to this list below
APP_OBJS = onlineupdate.o ./version_info/version.o ./crc/crc.o ./tcp_udp_lib/tcp_udp_lib.o
CLIENT_APP_OBJS = client.o ./version_info/version.o ./crc/crc.o ./tcp_udp_lib/tcp_udp_lib.o
MODIFY_APP_OBJS = modify_img.o
CFLAGS = -I ./ -I ./crc -I ./version_info -I ./tcp_udp_lib

all: build
build: $(APP)
$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(APP_OBJS) $(LDLIBS) -lpthread

build: $(CLIENT_APP)
$(CLIENT_APP): $(CLIENT_APP_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(CLIENT_APP_OBJS) $(LDLIBS) -lpthread

build: $(MODIFY_APP)
$(MODIFY_APP): $(MODIFY_APP_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(MODIFY_APP_OBJS) $(LDLIBS) -lpthread

clean:
	rm crc/*.o version_info/*.o tcp_udp_lib/*.o onlineupdate client_update modify_img *.o -rf


