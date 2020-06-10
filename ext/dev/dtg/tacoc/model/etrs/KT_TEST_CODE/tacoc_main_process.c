#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <wrapper/dtg_atcmd.h>
#include <board/board_system.h>
#include <board/modem-time.h>
#include <board/battery.h>
#include <util/nettool.h>

#include <standard_protocol.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>
#include <base/dmmgr.h>

#if defined(BOARD_TL500K)
	#include <common/kt_fota_inc/kt_fs_send.h>
	#include <common/kt_fota_inc/kt_fs_config.h>
	int g_kt_fota_dtg_no_rcv_count = 0;
#endif

void *do_dtg_report_thread()
{

	DTG_LOGD("%s: %s() --", __FILE__, __func__);
	while(1) {
		current_time = get_modem_time_utc_sec();

		if(current_time - report_time >= get_dtg_report_period()) {
			report_time = current_time;
		}
		else {
			sleep(10);
			continue;
		}

		
		if(key_status == POWER_IGNITION_ON)
		{
			send_cnt = 0;
			while(1) {
				if(send_cnt > 10)
				{
					DTG_LOGT("send count 10 times over.");
					break;
				}

				remain_count = tacom_unreaded_records_num();
				if(remain_count < 0) {
					break;
				}
				else
				{
					send_cnt += 1;
					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_dtg_report_period(), 0, 2);
					DTG_LOGI("%s:%d> ACCUMAL_DATA [%d]", __func__, __LINE__, ret);
					if (ret < 0) {
						DTG_LOGE("dtg data read error");
						if(ret == -2222) {
							DTG_LOGE("dtg data rserver send error");
							sleep(3);
						}
						break; //Error Next Time Retry..

					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
						break;
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
						if (ret < 0) {
							DTG_LOGE("cannot clear dtg buffer.");
							break;
						}
						sleep(3);
					}
				}
			} //end while

		}
	}
}


