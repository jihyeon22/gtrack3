MDT800_SRC = ext/protocol/mdt800/file_mileage.o \
             ext/protocol/mdt800/geofence.o \
             ext/protocol/mdt800/gps_filter.o \
             ext/protocol/mdt800/gps_utill.o \
             ext/protocol/mdt800/gpsmng.o \
             ext/protocol/mdt800/hdlc_async.o \
             ext/protocol/mdt800/packet.o \
             # ext/protocol/mdt800/sms.o \ # remove sms
       
OBJS_MODEL += $(MDT800_SRC)
CFLAGS += -I./ext/protocol/

OBJS_MODEL	+= $(MODEL_PATH)/netcom.o
OBJS_MODEL	+= $(MODEL_PATH)/callback.o

OBJS_MODEL	+= $(MODEL_PATH)/config.o
OBJS_MODEL	+= $(MODEL_PATH)/data-list.o
OBJS_MODEL	+= $(MODEL_PATH)/validation.o

OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o
OBJS_MODEL	+= $(MODEL_PATH)/thread-keypad.o

OBJS_MODEL	+= $(MODEL_PATH)/btn_key_mgr.o

OBJS_MODEL	+= $(MODEL_PATH)/mdt_pkt_senario.o
OBJS_MODEL	+= $(MODEL_PATH)/dtg_pkt_senario.o
OBJS_MODEL	+= $(MODEL_PATH)/dvr_pkt_senario.o

OBJS_MODEL	+= $(MODEL_PATH)/sms.o

## dsic protocol
OBJS_MODEL	+= ext/protocol/dtg/dsic/dsic_dtg_pkt.o
CFLAGS += -I./ext/protocol/dtg/dsic/

