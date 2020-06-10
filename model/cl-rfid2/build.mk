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

############ ADAS ########################################
OBJS_MODEL	+= $(MODEL_PATH)/ext/adas/cl_adas_mgr.o

############ RFID #####################################
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cl_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cl_rfid_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cl_rfid_senario.o

ifeq ($(USE_KJTEC_RFID),y)
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/kjtec_rfid/kjtec_rfid_cmd.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/kjtec_rfid/kjtec_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/kjtec_rfid/kjtec_rfid_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/kjtec_rfid/kjtec_rfid_senario.o
endif

ifeq ($(USE_SUP_RFID),y)
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/sup_rfid/sup_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/sup_rfid/sup_rfid_cmd.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/sup_rfid/sup_rfid_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/sup_rfid/sup_rfid_senario.o
endif


ifeq ($(USE_CUST1_RFID),y)
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust1_rfid/cust1_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust1_rfid/cust1_rfid_cmd.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust1_rfid/cust1_rfid_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust1_rfid/cust1_rfid_senario.o
endif

ifeq ($(USE_CUST2_RFID),y)
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust2_rfid/cust2_rfid_tools.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust2_rfid/cust2_rfid_cmd.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust2_rfid/cust2_rfid_pkt.o
OBJS_MODEL	+= $(MODEL_PATH)/ext/rfid/cust2_rfid/cust2_rfid_senario.o
endif


