OBJS_MODEL	:= $(MODEL_PATH)/netcom.o $(MODEL_PATH)/callback.o $(MODEL_PATH)/sms.o $(MODEL_PATH)/config.o 
OBJS_MODEL	+= $(MODEL_PATH)/data-list.o $(MODEL_PATH)/katech-packet.o 
#OBJS_MODEL	+= $(MODEL_PATH)/seco_obd_2.o $(MODEL_PATH)/seco_obd_util_2.o
OBJS_MODEL	+= $(MODEL_PATH)/katech-data-calc.o
OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o

OBJS_MODEL	+= $(MODEL_PATH)/katech-tools.o
OBJS_MODEL	+= $(MODEL_PATH)/seco_obd_mgr.o
