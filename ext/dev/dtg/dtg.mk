# ===========================================================
# DTG COMMON CONFIG
# ===========================================================
ifeq ($(USE_DTG_MODEL),y)
DTG_COMMON_OBJ += ext/dev/dtg/tacoc/common_sms.o \
				  ext/dev/dtg/tacoc/common_mdmc.o \
				  ext/dev/dtg/tacoc/common_debug.o \
				  ext/dev/dtg/tacoc/tacoc_local.o \
				  ext/dev/dtg/tacoc/tacoc.o \

DTG_COMMON_OBJ += ext/dev/dtg/wrapper/dtg_atcmd.o \
				  ext/dev/dtg/wrapper/dtg_convtools.o \
				  ext/dev/dtg/wrapper/dtg_etc_api.o \
				  ext/dev/dtg/wrapper/dtg_log.o \
				  ext/dev/dtg/wrapper/dtg_mdmc_wrapper_rpc_clnt.o \
				  ext/dev/dtg/wrapper/dtg_taco_wrapper_rpc_clnt.o \
				  ext/dev/dtg/wrapper/dtg_tacoc_wrapper_rpc_clnt.o \

DTG_COMMON_OBJ += ext/dev/dtg/tacom/tacom_process.o \
				  ext/dev/dtg/tacom/tacom.o \
				  ext/dev/dtg/tacom/taco.o \
				  ext/dev/dtg/tacom/taco_local.o \

DTG_COMMON_CFLAGS += -I./ext/dev/dtg/include/ \
				     -I./ext/dev/dtg/tacom/ \
				     -I./ext/dev/dtg/tacom/model/ \
				     -I./ext/dev/dtg/tacoc/ \
				     -I./ext/dev/dtg/ \
					 -DDTG_ENV_VAL=\"DTG_UPD_STAT\" \
					 -DDTG_ENABLE 

DTG_CONFIG_PATH=ext/dev/dtg/tacoc/configs
endif

# ===========================================================
# DTG MODEL CONFIG
# ===========================================================
ifeq ($(DTG_MODEL),choyoung)
DTG_MODEL_OBJ := $(DTG_COMMON_OBJ)
DTG_MODEL_OBJ += ext/dev/dtg/tacom/model/tacom_choyoung.o 
DTG_MODEL_CFLAGS += $(DTG_COMMON_CFLAGS)
DTG_MODEL_CFLAGS += -DDEVICE_MODEL_CHOYOUNG 

#DEVMODEL="choyoung"
#DEVABBR="CY"
DTG_PKG_PREFIX="CY"
# ----------------------------------------------------
else ifeq ($(DTG_MODEL),innocar)
DTG_MODEL_OBJ := $(DTG_COMMON_OBJ)
DTG_MODEL_OBJ += ext/dev/dtg/tacom/model/tacom_innocar.o  ext/dev/dtg/tacom/tools/taco_store.o
DTG_MODEL_CFLAGS += $(DTG_COMMON_CFLAGS)
DTG_MODEL_CFLAGS += -DDEVICE_MODEL_INNOCAR

#DEVMODEL="innocar"
#DEVABBR="INC"
DTG_PKG_PREFIX="INC"
# ----------------------------------------------------
else ifeq ($(DTG_MODEL),lp2)
DTG_MODEL_OBJ := $(DTG_COMMON_OBJ)
DTG_MODEL_OBJ += ext/dev/dtg/tacom/model/tacom_new_loop.o  ext/dev/dtg/tacom/tools/taco_store.o
DTG_MODEL_CFLAGS += $(DTG_COMMON_CFLAGS)
DTG_MODEL_CFLAGS += -DDEVICE_MODEL_LOOP2
#DEVMODEL="lp2"
#DEVABBR="lp2"
DTG_PKG_PREFIX="LP2"
# ----------------------------------------------------
else ifeq ($(DTG_MODEL),skel)
DTG_MODEL_OBJ := $(DTG_COMMON_OBJ)
DTG_MODEL_OBJ += ext/dev/dtg/tacom/model/skel_model.o 
DTG_MODEL_CFLAGS += $(DTG_COMMON_CFLAGS)
DTG_MODEL_CFLAGS += -DSKEL_MODEL
# ----------------------------------------------------
else
$(error DTG_MODEL is not correct, please define correct DTG_MODEL)
endif

# ===========================================================
# DTG SERVER CONFIG
# ===========================================================
ifeq ($(DTG_SERVER),hnrt)
DTG_CONFIG_FILE=hnrt.ini
DTG_SERVER_OBJ += ext/dev/dtg/tacoc/model/hnrt/tacoc_api.o \
				  ext/dev/dtg/tacoc/model/hnrt/dtg_net_com.o \
				  ext/dev/dtg/tacoc/model/hnrt/parsing.o \
				  ext/dev/dtg/tacoc/model/hnrt/dtg_regist_process.o \
				  ext/dev/dtg/tacoc/model/hnrt/dtg_ini_utill.o \
				  ext/dev/dtg/tacoc/model/hnrt/dtg_data_manage.o \
				  ext/dev/dtg/tacoc/model/hnrt/vehicle_msg.o \
				  ext/dev/dtg/tacoc/model/hnrt/sms_msg_process.o \
				  ext/dev/dtg/tacoc/model/hnrt/rpc_clnt_operation.o \
				  ext/dev/dtg/tacoc/model/hnrt/tacoc_main_process.o 
DTG_SERVER_CFLAGS += -I./ext/dev/dtg/tacoc/model/hnrt/inc/
DTG_SERVER_CFLAGS += -DSERVER_MODEL_HNRT
# ----------------------------------------------------
else ifeq ($(DTG_SERVER),gtrs)
DTG_CONFIG_FILE=gtrs.ini
DTG_SERVER_OBJ += ext/dev/dtg/tacoc/model/gtrs/dtg_data_manage.o            \
				  ext/dev/dtg/tacoc/model/gtrs/dtg_ini_utill.o      \
				  ext/dev/dtg/tacoc/model/gtrs/dtg_net_com.o        \
				  ext/dev/dtg/tacoc/model/gtrs/dtg_regist_process.o \
				  ext/dev/dtg/tacoc/model/gtrs/rpc_clnt_operation.o \
				  ext/dev/dtg/tacoc/model/gtrs/sms_msg_process.o    \
				  ext/dev/dtg/tacoc/model/gtrs/tacoc_api.o          \
				  ext/dev/dtg/tacoc/model/gtrs/tacoc_main_process.o \
				  ext/dev/dtg/tacoc/model/gtrs/parsing.o

	ifeq ($(SERVER),dtg-skel)
		DTG_SERVER_OBJ += ext/dev/dtg/tacoc/model/gtrs/mdt_data_manage.o
	endif
	
DTG_SERVER_CFLAGS += -I./ext/dev/dtg/tacoc/model/gtrs/inc/
DTG_SERVER_CFLAGS += -DSERVER_MODEL_GTRS
# ----------------------------------------------------
else ifeq ($(DTG_SERVER),neognp)
DTG_CONFIG_FILE=neognp.ini
DTG_SERVER_OBJ += ext/dev/dtg/tacoc/model/gtrs/dtg_data_manage.o            \
				  ext/dev/dtg/tacoc/model/gtrs/dtg_ini_utill.o      \
				  ext/dev/dtg/tacoc/model/gtrs/dtg_net_com.o        \
				  ext/dev/dtg/tacoc/model/gtrs/dtg_regist_process.o \
				  ext/dev/dtg/tacoc/model/gtrs/rpc_clnt_operation.o \
				  ext/dev/dtg/tacoc/model/gtrs/sms_msg_process.o    \
				  ext/dev/dtg/tacoc/model/gtrs/tacoc_api.o          \
				  ext/dev/dtg/tacoc/model/gtrs/tacoc_main_process.o \
				  ext/dev/dtg/tacoc/model/gtrs/parsing.o
DTG_SERVER_CFLAGS += -I./ext/dev/dtg/tacoc/model/gtrs/inc/
DTG_SERVER_CFLAGS += -DSERVER_MODEL_NEOGNP
# ----------------------------------------------------
else ifeq ($(DTG_SERVER),moram)
DTG_CONFIG_FILE=moram.ini
DTG_SERVER_OBJ += ext/dev/dtg/tacoc/model/moram/dtg_data_manage.o            \
				  ext/dev/dtg/tacoc/model/moram/dtg_ini_utill.o      \
				  ext/dev/dtg/tacoc/model/moram/dtg_net_com.o        \
				  ext/dev/dtg/tacoc/model/moram/dtg_regist_process.o \
				  ext/dev/dtg/tacoc/model/moram/rpc_clnt_operation.o \
				  ext/dev/dtg/tacoc/model/moram/sms_msg_process.o    \
				  ext/dev/dtg/tacoc/model/moram/tacoc_api.o          \
				  ext/dev/dtg/tacoc/model/moram/tacoc_main_process.o \
				  ext/dev/dtg/tacoc/model/moram/parsing.o

	ifeq ($(SERVER),dtg-skel)
		DTG_SERVER_OBJ += ext/dev/dtg/tacoc/model/moram/mdt_data_manage.o
	endif
	
DTG_SERVER_CFLAGS += -I./ext/dev/dtg/tacoc/model/gtrs/inc/
DTG_SERVER_CFLAGS += -DSERVER_MODEL_MORAM
# ----------------------------------------------------
else
$(error DTG_SERVER is not correct, please define correct DTG_SERVER)
endif

# ===========================================================
# DTG CONFIG COPY TO MAIN!!
# ===========================================================
OBJ_DTG += $(DTG_MODEL_OBJ)
OBJ_DTG += $(DTG_SERVER_OBJ)

DTG_CFLAGS += -DUSE_DTG_MODEL
DTG_CFLAGS += $(DTG_MODEL_CFLAGS)
DTG_CFLAGS += $(DTG_SERVER_CFLAGS)


##################################################
#
##################################################