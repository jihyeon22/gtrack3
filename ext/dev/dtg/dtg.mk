
TACOM_SRC = ext/dev/dtg/tacom/tacom_process.o \
            ext/dev/dtg/tacom/tacom.o \
			ext/dev/dtg/tacom/taco.o \
			ext/dev/dtg/tacom/model/tacom_choyoung.o \
			ext/dev/dtg/tacom/taco_local.o \
			

TACOM_SRC += ext/dev/dtg/tacoc/common_sms.o \
			 ext/dev/dtg/tacoc/common_mdmc.o \
			 ext/dev/dtg/tacoc/common_debug.o \
			 ext/dev/dtg/tacoc/tacoc_local.o \

TACOM_SRC += ext/dev/dtg/tacoc/model/hnrt/tacoc_api.o \
			 ext/dev/dtg/tacoc/model/hnrt/dtg_net_com.o \
			 ext/dev/dtg/tacoc/model/hnrt/parsing.o \
			 ext/dev/dtg/tacoc/model/hnrt/dtg_regist_process.o \
			 ext/dev/dtg/tacoc/model/hnrt/dtg_ini_utill.o \
			 ext/dev/dtg/tacoc/model/hnrt/dtg_data_manage.o \
			 ext/dev/dtg/tacoc/model/hnrt/vehicle_msg.o \
			 ext/dev/dtg/tacoc/model/hnrt/sms_msg_process.o \
			 ext/dev/dtg/tacoc/model/hnrt/rpc_clnt_operation.o \

TACOM_SRC += ext/dev/dtg/wrapper/dtg_atcmd.o \
		     ext/dev/dtg/wrapper/dtg_convtools.o \
			 ext/dev/dtg/wrapper/dtg_etc_api.o \
			 ext/dev/dtg/wrapper/dtg_log.o \
			 ext/dev/dtg/wrapper/dtg_mdmc_wrapper_rpc_clnt.o \
			 ext/dev/dtg/wrapper/dtg_taco_wrapper_rpc_clnt.o \
			 ext/dev/dtg/wrapper/dtg_tacoc_wrapper_rpc_clnt.o \

			 

CFLAGS += -I./ext/dev/dtg/include/
CFLAGS += -I./ext/dev/dtg/tacom/
CFLAGS += -I./ext/dev/dtg/tacom/model/
CFLAGS += -I./ext/dev/dtg/tacoc/
CFLAGS += -I./ext/dev/dtg/
CFLAGS += -I./ext/dev/dtg/tacoc/model/hnrt/inc/

CFLAGS += -DDTG_ENABLE -DDEVICE_MODEL_CHOYOUNG -DSERVER_MODEL_HNRT -DDTG_ENV_VAL=\"DTG_UPD_STAT\"

# $(Q)fakeroot cp -v $(CONFIG_FILE) $(DESTDIR)$(WORK_PATH)


