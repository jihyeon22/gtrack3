include ext/protocol/mdt800/mdt800.mk

OBJS_MODEL	+= $(MODEL_PATH)/netcom.o
OBJS_MODEL	+= $(MODEL_PATH)/callback.o

OBJS_MODEL	+= $(MODEL_PATH)/config.o
OBJS_MODEL	+= $(MODEL_PATH)/data-list.o
OBJS_MODEL	+= $(MODEL_PATH)/validation.o

OBJS_MODEL	+= $(MODEL_PATH)/at_noti.o