.SUFFIXES : .c .o 

#CC = gcc
#CC = /opt/nicta/tools/gcc-3.4.4-glibc-2.3.5/arm-linux/bin/arm-linux-gcc
CC = /opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc


INC = ./
LDFLAGS_PATH ?= -L$(CURDIR)

CFLAGS = -I$(INC) -DTEST_CODE_ENABLE -Wall
LIBS = -lm -lrt -lpthread

OBJS = main.o
TARGET = tweding_test4

all : $(TARGET)
	
$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS_PATH)

clean :
	rm -rf $(OBJS) $(TARGET) *.bak ./inc/*.bak
