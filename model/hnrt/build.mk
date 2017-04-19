include ext/protocol/mdt800/mdt800.mk

OBJS_MODEL	+= $(MODEL_PATH)/netcom.o
OBJS_MODEL	+= $(MODEL_PATH)/callback.o

OBJS_MODEL	+= $(MODEL_PATH)/config.o
OBJS_MODEL	+= $(MODEL_PATH)/data-list.o
OBJS_MODEL	+= $(MODEL_PATH)/validation.o

OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o

#OBJS_MODEL	+= prtcl/mdt800/gpsmng.o
#OBJS_MODEL	+= prtcl/mdt800/gps_filter.o
#OBJS_MODEL	+= prtcl/mdt800/gps_utill.o
#OBJS_MODEL	+= prtcl/mdt800/packet.o
#OBJS_MODEL	+= prtcl/mdt800/geofence.o
#OBJS_MODEL	+= prtcl/mdt800/file_mileage.o
#OBJS_MODEL	+= prtcl/mdt800/hdlc_async.o
#OBJS_MODEL	+= prtcl/mdt800/sms.o

