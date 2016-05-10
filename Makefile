OBJS=sharemem_server.o
OBJS2=sharemem_client.o
CFLAGS=-Wall -O2
LDLIBS=-lpthread
TARGET=sharemem_server
TARGET2=sharemem_client

.SUFIXES: .c .o

.PHONY: all
all: $(TARGET) $(TARGET2)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(LDLIBS) $^
$(TARGET2):$(OBJS2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET2) $(LDLIBS) $^

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(OBJS2) $(TARGET2)

