#include ext/protocol/mdt800/mdt800.mk
# not use sms.o

OBJS_MODEL	+= $(MODEL_PATH)/netcom.o
OBJS_MODEL	+= $(MODEL_PATH)/callback.o

OBJS_MODEL	+= $(MODEL_PATH)/config.o
OBJS_MODEL	+= $(MODEL_PATH)/data-list.o
OBJS_MODEL	+= $(MODEL_PATH)/validation.o
OBJS_MODEL	+= $(MODEL_PATH)/sms.o

OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o

MDT800_SRC = ext/protocol/mdt800/file_mileage.o \
             ext/protocol/mdt800/geofence.o \
             ext/protocol/mdt800/gps_filter.o \
             ext/protocol/mdt800/gps_utill.o \
             ext/protocol/mdt800/gpsmng.o \
             ext/protocol/mdt800/hdlc_async.o \
             ext/protocol/mdt800/packet.o \

       
OBJS_MODEL += $(MDT800_SRC)
CFLAGS += -I./ext/protocol/