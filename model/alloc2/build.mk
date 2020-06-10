OBJS_MODEL	:= $(MODEL_PATH)/netcom.o $(MODEL_PATH)/callback.o $(MODEL_PATH)/sms.o $(MODEL_PATH)/config.o

OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o

OBJS_MODEL	+= $(MODEL_PATH)/alloc2_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/alloc2_senario.o
OBJS_MODEL	+= $(MODEL_PATH)/alloc2_obd_mgr.o
OBJS_MODEL	+= $(MODEL_PATH)/alloc2_bcm_mgr.o
OBJS_MODEL	+= $(MODEL_PATH)/alloc2_daily_info.o
OBJS_MODEL	+= $(MODEL_PATH)/alloc2_nettool.o
