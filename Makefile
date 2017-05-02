MANPATH=/usr/share/man/man1
PROGPATH=/usr/bin

SRC=./src
DST=./bin
MAN=./man

dir_guard=@mkdir -p $(DST)

CFLAGS=-g -Wall --std=gnu99 -lm
#  -Ofast cannot be used with openwrt, please comment the following line
CFLAGS+=-Ofast
#FLAGS=-DCPU_INFO

OBJS=$(DST)/cpu_info.o $(DST)/wireless.o

all: client server

$(DST)/%.o: $(SRC)/%.c
	$(dir_guard)
	$(CC) -c $< -o $@ $(CFLAGS)

client: $(OBJS)
	$(dir_guard)
	$(CC) $(OBJS) $(SRC)/pingclient.c -o $(DST)/bwping-client $(CFLAGS) $(FLAGS)

server: $(OBJS)
	$(dir_guard)
	$(CC) $(OBJS) $(SRC)/pingserver.c -o $(DST)/bwping-server $(CFLAGS) $(FLAGS)

clean:
	@rm -rf $(DST) $(SRC)/*~ $(SRC)/*.swp $(SRC)/client.DSYM $(SRC)/server.DSYM

zip: clean
	@zip -r bwping-udp.zip Makefile README $(SRC)/* $(MAN)/*

manual:
	@sudo install -g 0 -o 0 -m 0644 $(MAN)/bwping-*.1 $(MANPATH)
	@sudo gzip -f $(MANPATH)/bwping-*.1

install: all manual
	@sudo cp $(DST)/bwping-client $(PROGPATH)
	@sudo cp $(DST)/bwping-server $(PROGPATH)
	@sudo chmod 555 $(PROGPATH)/bwping-*

remove:
	@sudo rm -f $(PROGPATH)/bwping-client $(PROGPATH)/bwping-server
	@sudo rm -f $(MANPATH)/bwping-client.1.gz $(MANPATH)/bwping-server.1.gz

