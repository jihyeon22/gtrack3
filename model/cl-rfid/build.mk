OBJS_MODEL	:= $(MODEL_PATH)/netcom.o
OBJS_MODEL	+= $(MODEL_PATH)/callback.o
OBJS_MODEL	+= $(MODEL_PATH)/sms.o
OBJS_MODEL	+= $(MODEL_PATH)/config.o

OBJS_MODEL	+= $(MODEL_PATH)/data-list.o
OBJS_MODEL	+= $(MODEL_PATH)/validation.o
OBJS_MODEL	+= $(MODEL_PATH)/command.o
OBJS_MODEL	+= $(MODEL_PATH)/geofence.o
OBJS_MODEL	+= $(MODEL_PATH)/section.o
OBJS_MODEL	+= $(MODEL_PATH)/routetool.o
#OBJS_MODEL	+= board/rfidtool.o

OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o
OBJS_MODEL	+= $(MODEL_PATH)/cl_mdt_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/cl_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/cl_rfid_pkt.o
ifeq ($(USE_KJTEC_RFID),y)
OBJS_MODEL	+= $(MODEL_PATH)/kjtec_rfid_cmd.o
OBJS_MODEL	+= $(MODEL_PATH)/kjtec_rfid_tools.o
endif
OBJS_MODEL	+= $(MODEL_PATH)/cl_rfid_senario.o
OBJS_MODEL	+= $(MODEL_PATH)/cl_adas_mgr.o

ifeq ($(USE_SUP_RFID),y)
OBJS_MODEL	+= $(MODEL_PATH)/sup_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/sup_rfid_cmd.o
endif