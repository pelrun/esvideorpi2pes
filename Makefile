CC      = gcc
CFLAGS  += -g -O3 -MD -Wall -I. -I../../include -D_FILE_OFFSET_BITS=64 $(CPPFLAGS)

OBJS = esvideorpi2pes.o nal.o
TARGET = esvideorpi2pes
DESTDIR ?= /usr/local/bin/

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

install: all
	install -m 755 $(TARGET) $(DESTDIR)

clean:
	rm -f $(TARGET) $(OBJS) core* *~ *.d

-include $(wildcard *.d) dummy

