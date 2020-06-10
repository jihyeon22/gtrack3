

TARGET          = taco
SOCK            = TACO
INC_PATH        += -I.

TACOM_SRC       = tacom.c         \
                  tacom_process.c \



TACO_SVC        += taco.c         \
                   taco_serv.c    \
                   taco_local.c   \
                   xdr/taco_xdr.c
   		   

TACO_COMMON     = $(SRCROOT)/common/convtools.c   

SVC_OBJS        = $(TACO_SVC:.c=.o)     \
                  $(TACO_COMMON:.c=.o)  \
                  $(TACOM_SRC:.c=.o)

CLNT_OBJS       = $(LOGD_CLNT:.c=.o)    \
                  $(TACOC_CLNT:.c=.o)   \
                  $(MOND_CLNT:.c=.o) 

OBJS            = $(SVC_OBJS) $(CLNT_OBJS)

ifeq ($(BOARD),NEO_W200)
TACO_COMMON         += $(SRCROOT)/common/w200_led.c
else ifeq ($(BOARD),NEO_W200K)
TACO_COMMON         += $(SRCROOT)/common/w200_led.c
endif

ifeq ("$(DEVICE_MODEL)", "ucar")
	TACOM_SRC	+= tacom_ucar.c
else ifeq ("$(DEVICE_MODEL)", "sinhung")
	TACOM_SRC	+= tacom_sinhung.c
else ifeq ("$(DEVICE_MODEL)", "loop")
	TACOM_SRC	+= tacom_loop.c
else ifeq ("$(DEVICE_MODEL)", "loop2")
	TACOM_SRC	+= tacom_new_loop.c
else ifeq ("$(DEVICE_MODEL)", "choyoung")
	TACOM_SRC	+= tacom_choyoung.c
else ifeq ("$(DEVICE_MODEL)", "kdt")
	TACOM_SRC	+= tacom_choyoung.c
else ifeq ("$(DEVICE_MODEL)", "ireal")
	TACOM_SRC	+= ireal.c tacom_ireal.c
else ifeq ("$(DEVICE_MODEL)", "innocar")
	TACOM_SRC	+= tacom_innocar.c taco_store.c
else ifeq ("$(DEVICE_MODEL)", "innosns")
	TACOM_SRC	+= tacom_innocar_opensns.c iniutill.c taco_store.c mdt_store.c
	LDLIBS += -liniparser
else ifeq ("$(DEVICE_MODEL)", "innosns_dcu")
	TACOM_SRC	+= tacom_innocar_opensns.c iniutill.c taco_store.c mdt_store.c
	LDLIBS += -liniparser
else ifeq ("$(DEVICE_MODEL)", "cj")
	TACOM_SRC	+= tacom_cj.c
else ifeq ("$(DEVICE_MODEL)", "daesin")
	TACOM_SRC	+= tacom_daesin.c
endif



CFLAGS += -DDTG_SOCKNAME=\"$(TEMP_PATH)/$(SOCK)\"
CFLAGS += $(INC_PATH) -g
CFLAGS += -DBOARD_$(BOARD)
LDLIBS += -lnsl -lpthread -lm -rdynamic

all : $(OBJS)
	$(eval $(call checkmodel))
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

install: $(TARGET)
	$(eval $(call checkmodel))
	@cp -v $(TARGET) $(DESTDIR)$(WORK_PATH)/$(TARGET)
	$(STRIP) $(DESTDIR)$(WORK_PATH)/$(TARGET)

clean:
	@rm -rf $(OBJS) $(TARGET)
	rm -rf *.o
