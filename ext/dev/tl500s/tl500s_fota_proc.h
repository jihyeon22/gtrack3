#ifndef __TL500S_FOTA_PROC_H__
#define __TL500S_FOTA_PROC_H__

#define TL500S_TARGET_ORIGINAL_IMG_VERSION  "TL500S_1.1.0 [Feb 21 2017 10:06:35]"
#define TL500S_FOTA_SCRIPT_PATH             "/system/mds/system/bin/tld_fota_tl500s.sh"

#define MODEM_VER_CHK_MAX_RETRY     5

int tl500s_fota_proc();

#endif

