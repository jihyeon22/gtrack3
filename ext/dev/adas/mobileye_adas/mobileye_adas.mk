

MOBILEYE_ADAS_CFLAGS  = -Iext/dev/adas/mobileye_adas/
MOBILEYE_ADAS_CFLAGS += -Iext/dev/adas/mobileye_adas/include
MOBILEYE_ADAS_CFLAGS += -Iext/dev/adas/common/include

OBJ_MOBILEYE_ADAS  = ext/dev/adas/mobileye_adas/src/mobileye_adas_mgr.o
OBJ_MOBILEYE_ADAS += ext/dev/adas/mobileye_adas/src/mobileye_adas_protocol.o
OBJ_MOBILEYE_ADAS += ext/dev/adas/mobileye_adas/src/mobileye_adas_tool.o
OBJ_MOBILEYE_ADAS += ext/dev/adas/mobileye_adas/src/mobileye_adas_uart_util.o




