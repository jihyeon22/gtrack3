#include ext/protocol/mdt800/mdt800.mk
# not use sms.o

OBJS_MODEL	+= $(MODEL_PATH)/netcom.o
OBJS_MODEL	+= $(MODEL_PATH)/callback.o

OBJS_MODEL	+= $(MODEL_PATH)/config.o
#OBJS_MODEL	+= $(MODEL_PATH)/data-list.o
#OBJS_MODEL	+= $(MODEL_PATH)/validation.o


OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o

# OBJS_MODEL	+= $(MODEL_PATH)/ktth_senario.o

OBJS_MODEL	+= $(MODEL_PATH)/kt_flood_unit.o
#OBJS_MODEL	+= $(MODEL_PATH)/flood_unit.o


OBJS_MODEL	+= $(MODEL_PATH)/leakdata-list.o
OBJS_MODEL	+= $(MODEL_PATH)/packetdata_util.o
OBJS_MODEL	+= $(MODEL_PATH)/transfer_flood.o

#OBJS_MODEL	+= $(MODEL_PATH)/flood_unit.o



MDT800_SRC = $(MODEL_PATH)/kt_flood_mdt800/file_mileage.o \
             $(MODEL_PATH)/kt_flood_mdt800/geofence.o \
             $(MODEL_PATH)/kt_flood_mdt800/gps_filter.o \
             $(MODEL_PATH)/kt_flood_mdt800/gps_utill.o \
             $(MODEL_PATH)/kt_flood_mdt800/gpsmng.o \
             $(MODEL_PATH)/kt_flood_mdt800/hdlc_async.o \
             $(MODEL_PATH)/kt_flood_mdt800/packet.o \
             $(MODEL_PATH)/kt_flood_mdt800/sms.o \

       
OBJS_MODEL += $(MDT800_SRC)
