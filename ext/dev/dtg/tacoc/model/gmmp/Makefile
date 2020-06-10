.SUFFIXES : .c .o 

PLATFROM = X86_PLATROM
#PLATFROM = ARM_PLATROM


ifeq ("$(PLATFROM)", "X86_PLATROM")
	CC = gcc
	STRIP = strip
	LIB = ../lib.x86
	PLATFROM = X86_PLATROM
else
	CC = /opt/nicta/gcc-3.4.4-glibc-2.3.5/arm-linux/bin/arm-linux-gcc
	STRIP = /opt/nicta/gcc-3.4.4-glibc-2.3.5/arm-linux/bin/arm-linux-strip
	LIB = ../lib.arm
endif


INC = -I../inc/zlib -I./inc
LIBS = -lpthread -lrt -lz

#==========================================================================================================
# REQUEST_FIRST_ONE_BYTE_READ : request를 한 후 바로 read를 하지 않으면 rx로 아무 data도 들어 오지 않는다.
#
# SELF_UART_TEST is for uart test
#
# MULTI_BYTE_RECEIVE is received 128byte at one time
# if MULTI_BYTE_RECEIVE isn't define, receive 1 byte at one time
#
#==========================================================================================================
CFLAGS := $(INC) -L$(LIB) -DTACOC_STANDALONE
#CFLAGS := $(INC) -L$(LIB) -DTACOC_STANDALONE -DSMS_TEST

OBJS = main.o gmmp_utill.o gmmp_net.o gmmp_api.o gmmp_debug.o gmmp_manager.o gmmp_base64.o gmmp_dump.o iniparser.o dictionary.o

TARGET = test_gmmp


all : $(TARGET)
	
$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	$(STRIP) $(TARGET)

install : 
	mv $(TARGET)	../bin

clean :
	rm -rf $(OBJS) $(TARGET) *.bak ./inc/*.bak .inc/zlib/*.bak
