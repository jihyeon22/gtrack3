
OBJ_TX500S_FOTA_SH_PATH = ext/dev/tl500s
OBJ_TX500S_FOTA_SH_FILE = tld_fota_tl500s.sh

OBJ_TX500S_FOTA_CFLAGS = -Iext/dev/tl500s
OBJ_TX500S_FOTA_CFLAGS += -DTX500S_FOTA_SH_PATH=\"$(OBJ_TX500S_FOTA_SH_PATH)\$(OBJ_TX500S_FOTA_SH_FILE)\"

OBJ_TX500S_FOTA_OBJS = ext/dev/tl500s/tl500s_fota_proc.o