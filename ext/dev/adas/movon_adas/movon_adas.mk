MOVON_ADAS_CFLAGS = -Iext/dev/adas/movon_adas/
MOVON_ADAS_CFLAGS += -Iext/dev/adas/movon_adas/include
MOVON_ADAS_CFLAGS += -Iext/dev/adas/common/include

OBJ_MOVON_ADAS  = ext/dev/adas/movon_adas/src/movon_adas_mgr.o
OBJ_MOVON_ADAS += ext/dev/adas/movon_adas/src/movon_adas_protocol.o
OBJ_MOVON_ADAS += ext/dev/adas/movon_adas/src/movon_adas_tool.o
OBJ_MOVON_ADAS += ext/dev/adas/movon_adas/src/movon_adas_uart_util.o
OBJ_MOVON_ADAS += ext/dev/adas/movon_adas/src/movon_data_queue.o


