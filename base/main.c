<<<<<<< HEAD
/*===========================================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <board/board_system.h>
#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#ifdef USE_GPS_FAKE
#include <base/gpstool.h>
#include <util/gps_fake.h>
#endif
#include <base/thread.h>
#include <board/power.h>
#include <board/led.h>
#include <board/gpio.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/error.h>
#include <base/dmmgr.h>
#include <base/watchdog.h>
#include <board/battery.h>
#include <board/app-ver.h>
#include <board/crit-data.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/nettool.h>
#include <util/stackdump.h>
#include <util/poweroff.h>
#include <util/pipe.h>

#include <callback.h>
#include <config.h>

#include <at/at_util.h>
#include <logd_rpc.h>
#include <sms.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE

void _deinit_essential_functions(void)
{
#ifdef USE_GPS_MODEL
	mileage_write();
	gps_valid_data_write();
#endif

#ifdef USE_GPS_FAKE
	//mileage_write();
	gps_valid_data_write();
#endif

}

static void _deinit_resource(void)
{
	int i = 0;
	int status_thread_join = 0;
	int thread_cnt = 0;
	
	_deinit_essential_functions();

	terminate_app_callback();
	
	for ( i = 0 ; i < MAX_THREAD_COUNT ; i++ )
	{
		if (threads[i].thread == NULL)
			break;
		else
			thread_cnt++;
	}
			
	
	
	for( i = 0; i < thread_cnt; i++) {
		if (pthread_join(tid[i], (void**)&status_thread_join) < 0) {
			error_critical(eERROR_LOG, "pthread_join Error");
		}
	}

	dmmgr_deinit();

	sender_deinit();
	battery_deinit_adc();

	//To do : need led scenario for eError_hang. must have powerd.
	switch(gError_state)
	{
		case eERROR_EXIT:
		{
			error_rm_no_mon();
			break;
		}
		case eERROR_REBOOT:
		{
			poweroff("Critical Error reboot", sizeof("Critical Error reboot"));
			break;
		}
		case eERROR_FINAL:
		{
			while(1)
			{
				LOGE(LOG_TARGET, "Critical Error overrun\n");

				led_set_all(eCOLOR_RED, 1);
				sleep(1);
				led_set_all(eCOLOR_RED, 0);
				sleep(1);

				if(power_get_ignition_status() == POWER_IGNITION_OFF)
				{
					poweroff("Critical Error overrrun-ignition off", sizeof("Critical Error overrrun-ignition off"));
				}
			}
			break;
		}
		default:
			LOGE(LOG_TARGET, "gError_state is abnormal!\n");
		
	}
}

static void _power_check(void)
{
	int power_gpio_res = 0;
	configurationBase_t *conf = NULL;
	int n_try = 30;
	
	conf = get_config_base();	

	if(conf->common.bootstrap == 1)
	{
		return;
	}
	
	if(conf->common.initial_turnoff == 0)
	{
		return;
	}
	
	// if power source is battery, turn off device.

	while(n_try-- > 0)
	{
		power_gpio_res = power_get_power_source();
		if(power_gpio_res == POWER_SRC_DC)
		{
			break;
		}
		sleep(1);
	}

	if(power_gpio_res == POWER_SRC_BATTERY)
	{
		poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	}
}

#define MAX_TRY_GET_IMEI 5
#define MAX_TRY_GET_PHONENUM 5

static void _get_essential_data_blocking(void)
{
	int count_error = 0;
	char temp_buff[128] = {0,};
	
	while(count_error++ < MAX_TRY_GET_IMEI)
	{
		_power_check();
		memset(temp_buff, 0x00, 128);
		if(at_get_imei(temp_buff, AT_LEN_IMEI_BUFF) == 0) {
			break;
		}
		else
		{
			sleep(1);
			continue;
		}
	}
	if(count_error >= MAX_TRY_GET_IMEI)
	{
		error_critical(eERROR_REBOOT, "get_imei error");
	}

	count_error = 0;
	while(count_error++ < MAX_TRY_GET_PHONENUM)
	{
		_power_check();
		memset(temp_buff, 0x00, 128);
		if(at_get_phonenum(temp_buff, 11 + 1) == 0) {
			break;
		}
		else
		{
			sleep(1);
			continue;
		}
	}
	if(count_error >= MAX_TRY_GET_PHONENUM)
	{
		error_critical(eERROR_REBOOT, "get_phonum error");
	}

}

void _set_tz(void)
{
	// time zone 관련설정은 skip 한다.
	// 기존에 시스템 세팅값이 꼬이는지 실제 로그를 보면 시간값이 이상하다.
	//setenv("TZ", "KST-9", 1);
	//tzset();
}

void _initial_btn_check(void)
{
	int key1 = 0;
	int key2 = 0;
	int num_push = 0;

	while(1)
	{
		key1 = gpio_get_value(17);
		key2 = gpio_get_value(9);

		if(key1 != 0 && key2 != 0)
		{
			return;
		}

		num_push++;
		if(num_push >= 30)
		{
			led_noti(eLedcmd_INIT_NOTI);
			sleep(5);
		
			//reset data
			tools_rm_all(USER_DATA_DIR);

			crit_init();
			crit_clear_write();
			
			poweroff(__FUNCTION__, sizeof(__FUNCTION__));
		}
		
		usleep(300000);
	}
}

void _gpio_check(void)
{
	int n_try = 10;
	
	while(n_try--)
	{
		if(power_get_ignition_status() >= 0 && power_get_power_source() >= 0)
		{
			break;
		}
		
		sleep(1);
	}
	
	if(n_try < 0)
	{
		poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	}
}

void gtrack_at_noti_proc(const char* buffer, int len)
{
	int remove_cr_str_len = 0;
	char temp_str[1024] = {0,};
	char* filtered_sms = NULL;
	
	remove_cr_str_len = mds_api_remove_cr(buffer, temp_str, 512);

	if ( remove_cr_str_len <= 0 )
	{
		LOGE(LOG_TARGET, "model at noti proc msg (remove cr) : null ==> ori msg [%s] remove cr len [%d]\n", buffer, remove_cr_str_len);
		return;
	}

	filtered_sms = temp_str;
	// remove "[web....]"
	//printf(" strncasecmp => [%c] [%d]\r\n", target[0], strncasecmp(target+1,"web", strlen("web") ) );
	if (( temp_str[0] == '[' ) && ( strncasecmp(temp_str+1,"web", strlen("web") )  == 0 ) )
	{
		char* tmp_char = strstr(temp_str+4,"]");

		if ( tmp_char != NULL )
		{
			tmp_char++;
			filtered_sms = tmp_char;
		}
	}

	LOGI(LOG_TARGET, "model at noti proc msg (remove cr) ==> [%s] [%d]\n", filtered_sms, strlen(filtered_sms));

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
	KT_FOTA_NOTI_RECEIVE(temp_str);
#endif
	parse_model_at_noti(filtered_sms, strlen(filtered_sms));
}

void gtrack_at_sms_proc(const char* phone_num, const char* recv_time, const char* msg)
{
	int remove_cr_str_len = 0;
	char temp_str[128] = {0,};

	char* filtered_sms = NULL;

	remove_cr_str_len = mds_api_remove_cr(msg, temp_str, 128);

	if ( remove_cr_str_len <= 0 )
	{
		LOGE(LOG_TARGET, "model sms proc msg (remove cr) : null ==> ori msg [%s] remove cr len [%d]\n", msg, remove_cr_str_len);
		return;
	}

	filtered_sms = temp_str;
	// remove "[web....]"
	//printf(" strncasecmp => [%c] [%d]\r\n", target[0], strncasecmp(target+1,"web", strlen("web") ) );
	if (( temp_str[0] == '[' ) && ( strncasecmp(temp_str+1,"web", strlen("web") )  == 0 ) )
	{
		char* tmp_char = strstr(temp_str+4,"]");

		if ( tmp_char != NULL )
		{
			tmp_char++;
			filtered_sms = tmp_char;
		}
	}

	LOGI(LOG_TARGET, "model sms proc msg (remove cr) ==> [%s] [%d]\n", filtered_sms, strlen(filtered_sms));
	//int parse_model_sms(char *time, char *phonenum, char *sms);
	parse_model_sms(recv_time, phone_num, filtered_sms);

#ifdef DTG_ENABLE
	tx_sms_to_tacoc(phone_num, temp_str);
#endif
}

int main(int argc, char** argv)
{
	pid_t pid = -1;
	configurationBase_t *conf_base = NULL;

	//user directory create ++
	while(1) {
		if(tools_check_exist_file(USER_DATA_DIR, 2) >= 0) {
			break;
		}
		printf("User dir : %s can't find\n");
		system(MKDIR_USER_DIR);
		sleep(1);
	}
	//user directory create --

	stackdump_init();
	logd_init();

	LOGI(LOG_TARGET, "Build date:" __DATE__ __TIME__ "\n");
	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if((pid = fork()) < 0) {
		error_critical(eERROR_EXIT, "fork Error");
	} else if(pid != 0) {
		exit(0);
	}
	
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	setsid();

	if(sender_init() < 0)
	{
		LOGE(LOG_TARGET, "sender_init  Error\n");
		error_critical(eERROR_EXIT, "sender_init Error");
	}

	//at_open(e_DEV_TX501_BASE, NULL, NULL, NULL);
	//at_open(e_DEV_TX501_BASE, NULL, NULL, "console");
	at_open(AT_LIB_TARGET_DEV, gtrack_at_noti_proc, gtrack_at_sms_proc, "console");
	

	//at_start();

	//_initial_btn_check();

	//led_noti(eLedcmd_SYSTEM_INIT);
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L) 
	_gpio_check();
#endif

	error_rm_no_mon();

	if(argc == 2 && strcmp(argv[1], "-g") == 0)
	{
	#ifdef USE_GPS_MODEL
		if(gps_set_pipe_for_emul() >= 0)
			LOGT(LOG_TARGET, "Enable GPS Emulation!\n");
	#endif
	}

	if((load_config_base()) == NULL) {
		LOGE(LOG_TARGET, "load_config_base Error\n");
		error_critical(eERROR_LOG, "load_config_base Error");
		load_config_base_default();
	}

	if(load_config_model() == NULL) {
		LOGE(LOG_TARGET, "load_config_model Error\n");
		error_critical(eERROR_LOG, "load_config_model Error");
		load_config_model_default();
	}

	if(load_config_user() == NULL) {
		LOGE(LOG_TARGET, "load_config_user Error\n");
		error_critical(eERROR_LOG, "load_config_user Error");
		load_config_user_default();
	}

	conf_base = get_config_base();

//	_power_check();
	
	_set_tz();

	dmmgr_init();

	crit_init();

#ifdef USE_GPS_MODEL
	mileage_read();

	gps_valid_data_read();

	// agps setting for tl500
	at_set_gps_on(e_GPS_ON_TYPE_SET_ENV_AGPS, e_GPS_BOOT_TYPE_NULL);
#endif

#ifdef USE_GPS_FAKE
   // mileage_read();
    gps_valid_data_read();
#endif

	if(battery_init_adc() < 0)
	{
		LOGE(LOG_TARGET, "battery_init_adc Error\n");
		error_critical(eERROR_EXIT, "battery_init_adc Error");
	}

    init_model_callback();
    // kksworks : force setting for iot KT cert.
#ifdef KT_FOTA_TEST_SVR
	thread_network_set_warn_timeout(0); // waring msg disable..
    set_auto_ota_tl500(0); // auto ota disable.
    set_max_network_fail_reset_cnt(0); // network invalid chk disable..
#endif

	stackdump_abort_base_callback = _deinit_essential_functions;

/*
#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)

	int stat_num = 0;
	char buf[512];
	if(at_get_state(&stat_num, buf, sizeof(buf)) >= 0) 
	{
		printf("stat_num = [%d]\n", stat_num);
		if(stat_num == 0) 
		{
			if(!strncmp(buf, "OPEN", 4)) //not service available
			{
				wcdma_error_led_notification();
			}
		}
		else if(stat_num == 14) //usim not detect
		{
			wcdma_error_led_notification();
		}
	}
#endif
*/

//	at_set_alive(conf_base->common.alive_time_sec);
//	_get_essential_data_blocking();

	watchdog_set_cur_ktime(eWdMain);
	watchdog_process();

	int i = 0;
	pthread_attr_t attr;

	for(i = 0; i < MAX_THREAD_COUNT; i++) {
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, threads[i].stack_size);
		
		if (threads[i].thread == NULL)
			break;
			
		if(pthread_create(&tid[i], &attr, threads[i].thread, NULL) != 0) {
			LOGE(LOG_TARGET, "Create Thread Fail.. exit Program...\r\n");
			error_critical(eERROR_EXIT, "pthread_create Error");
		}

		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();
		
		usleep(100000);
	}

	// model thread..
	if(thread_model != NULL)
	{
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 256*1024);
		if(pthread_create(&tid_model, &attr, (void *)thread_model, NULL) != 0) {
			LOGE(LOG_TARGET, "Create Thread Fail.. exit Program...\r\n");
			error_critical(eERROR_EXIT, "pthread_create Error");
		}
	}

	LOGT(LOG_TARGET, "INIT Complete.\n");

	watchdog_set_init_ktime(eWdMain);

#ifdef DTG_ENABLE 
	tacom_init();
	tacoc_run();
#endif

	main_loop_callback();

	_deinit_resource();
	
	return 0;
}

=======
/*===========================================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <board/board_system.h>
#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#ifdef USE_GPS_FAKE
#include <base/gpstool.h>
#include <util/gps_fake.h>
#endif
#include <base/thread.h>
#include <board/power.h>
#include <board/led.h>
#include <board/gpio.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/error.h>
#include <base/dmmgr.h>
#include <base/watchdog.h>
#include <board/battery.h>
#include <board/app-ver.h>
#include <board/crit-data.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/nettool.h>
#include <util/stackdump.h>
#include <util/poweroff.h>
#include <util/pipe.h>

#include <callback.h>
#include <config.h>

#include <at/at_util.h>
#include <logd_rpc.h>
#include <sms.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE

void _deinit_essential_functions(void)
{
#ifdef USE_GPS_MODEL
	mileage_write();
	gps_valid_data_write();
#endif

#ifdef USE_GPS_FAKE
	//mileage_write();
	gps_valid_data_write();
#endif

}

static void _deinit_resource(void)
{
	int i = 0;
	int status_thread_join = 0;
	int thread_cnt = 0;
	
	_deinit_essential_functions();

	terminate_app_callback();
	
	for ( i = 0 ; i < MAX_THREAD_COUNT ; i++ )
	{
		if (threads[i].thread == NULL)
			break;
		else
			thread_cnt++;
	}
			
	
	
	for( i = 0; i < thread_cnt; i++) {
		if (pthread_join(tid[i], (void**)&status_thread_join) < 0) {
			error_critical(eERROR_LOG, "pthread_join Error");
		}
	}

	dmmgr_deinit();

	sender_deinit();
	battery_deinit_adc();

	//To do : need led scenario for eError_hang. must have powerd.
	switch(gError_state)
	{
		case eERROR_EXIT:
		{
			error_rm_no_mon();
			break;
		}
		case eERROR_REBOOT:
		{
			poweroff("Critical Error reboot", sizeof("Critical Error reboot"));
			break;
		}
		case eERROR_FINAL:
		{
			while(1)
			{
				LOGE(LOG_TARGET, "Critical Error overrun\n");

				led_set_all(eCOLOR_RED, 1);
				sleep(1);
				led_set_all(eCOLOR_RED, 0);
				sleep(1);

				if(power_get_ignition_status() == POWER_IGNITION_OFF)
				{
					poweroff("Critical Error overrrun-ignition off", sizeof("Critical Error overrrun-ignition off"));
				}
			}
			break;
		}
		default:
			LOGE(LOG_TARGET, "gError_state is abnormal!\n");
		
	}
}

static void _power_check(void)
{
	int power_gpio_res = 0;
	configurationBase_t *conf = NULL;
	int n_try = 30;
	
	conf = get_config_base();	

	if(conf->common.bootstrap == 1)
	{
		return;
	}
	
	if(conf->common.initial_turnoff == 0)
	{
		return;
	}
	
	// if power source is battery, turn off device.

	while(n_try-- > 0)
	{
		power_gpio_res = power_get_power_source();
		if(power_gpio_res == POWER_SRC_DC)
		{
			break;
		}
		sleep(1);
	}

	if(power_gpio_res == POWER_SRC_BATTERY)
	{
		poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	}
}

#define MAX_TRY_GET_IMEI 5
#define MAX_TRY_GET_PHONENUM 5

static void _get_essential_data_blocking(void)
{
	int count_error = 0;
	char temp_buff[128] = {0,};
	
	while(count_error++ < MAX_TRY_GET_IMEI)
	{
		_power_check();
		memset(temp_buff, 0x00, 128);
		if(at_get_imei(temp_buff, AT_LEN_IMEI_BUFF) == 0) {
			break;
		}
		else
		{
			sleep(1);
			continue;
		}
	}
	if(count_error >= MAX_TRY_GET_IMEI)
	{
		error_critical(eERROR_REBOOT, "get_imei error");
	}

	count_error = 0;
	while(count_error++ < MAX_TRY_GET_PHONENUM)
	{
		_power_check();
		memset(temp_buff, 0x00, 128);
		if(at_get_phonenum(temp_buff, 11 + 1) == 0) {
			break;
		}
		else
		{
			sleep(1);
			continue;
		}
	}
	if(count_error >= MAX_TRY_GET_PHONENUM)
	{
		error_critical(eERROR_REBOOT, "get_phonum error");
	}

}

void _set_tz(void)
{
	// time zone 관련설정은 skip 한다.
	// 기존에 시스템 세팅값이 꼬이는지 실제 로그를 보면 시간값이 이상하다.
	//setenv("TZ", "KST-9", 1);
	//tzset();
}

void _initial_btn_check(void)
{
	int key1 = 0;
	int key2 = 0;
	int num_push = 0;

	while(1)
	{
		key1 = gpio_get_value(17);
		key2 = gpio_get_value(9);

		if(key1 != 0 && key2 != 0)
		{
			return;
		}

		num_push++;
		if(num_push >= 30)
		{
			led_noti(eLedcmd_INIT_NOTI);
			sleep(5);
		
			//reset data
			tools_rm_all(USER_DATA_DIR);

			crit_init();
			crit_clear_write();
			
			poweroff(__FUNCTION__, sizeof(__FUNCTION__));
		}
		
		usleep(300000);
	}
}

void _gpio_check(void)
{
	int n_try = 10;
	
	while(n_try--)
	{
		if(power_get_ignition_status() >= 0 && power_get_power_source() >= 0)
		{
			break;
		}
		
		sleep(1);
	}
	
	if(n_try < 0)
	{
		poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	}
}

void gtrack_at_noti_proc(const char* buffer, int len)
{
	int remove_cr_str_len = 0;
	char temp_str[1024] = {0,};
	char* filtered_sms = NULL;
	
	remove_cr_str_len = mds_api_remove_cr(buffer, temp_str, 512);

	if ( remove_cr_str_len <= 0 )
	{
		LOGE(LOG_TARGET, "model at noti proc msg (remove cr) : null ==> ori msg [%s] remove cr len [%d]\n", buffer, remove_cr_str_len);
		return;
	}

	filtered_sms = temp_str;
	// remove "[web....]"
	//printf(" strncasecmp => [%c] [%d]\r\n", target[0], strncasecmp(target+1,"web", strlen("web") ) );
	if (( temp_str[0] == '[' ) && ( strncasecmp(temp_str+1,"web", strlen("web") )  == 0 ) )
	{
		char* tmp_char = strstr(temp_str+4,"]");

		if ( tmp_char != NULL )
		{
			tmp_char++;
			filtered_sms = tmp_char;
		}
	}

	LOGI(LOG_TARGET, "model at noti proc msg (remove cr) ==> [%s] [%d]\n", filtered_sms, strlen(filtered_sms));

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
	KT_FOTA_NOTI_RECEIVE(temp_str);
#endif
	parse_model_at_noti(filtered_sms, strlen(filtered_sms));
}

void gtrack_at_sms_proc(const char* phone_num, const char* recv_time, const char* msg)
{
	int remove_cr_str_len = 0;
	char temp_str[128] = {0,};

	char* filtered_sms = NULL;

	remove_cr_str_len = mds_api_remove_cr(msg, temp_str, 128);

	if ( remove_cr_str_len <= 0 )
	{
		LOGE(LOG_TARGET, "model sms proc msg (remove cr) : null ==> ori msg [%s] remove cr len [%d]\n", msg, remove_cr_str_len);
		return;
	}

	filtered_sms = temp_str;
	// remove "[web....]"
	//printf(" strncasecmp => [%c] [%d]\r\n", target[0], strncasecmp(target+1,"web", strlen("web") ) );
	if (( temp_str[0] == '[' ) && ( strncasecmp(temp_str+1,"web", strlen("web") )  == 0 ) )
	{
		char* tmp_char = strstr(temp_str+4,"]");

		if ( tmp_char != NULL )
		{
			tmp_char++;
			filtered_sms = tmp_char;
		}
	}

	LOGI(LOG_TARGET, "model sms proc msg (remove cr) ==> [%s] [%d]\n", filtered_sms, strlen(filtered_sms));
	//int parse_model_sms(char *time, char *phonenum, char *sms);
	parse_model_sms(recv_time, phone_num, filtered_sms);

#ifdef DTG_ENABLE
	tx_sms_to_tacoc(phone_num, temp_str);
#endif
}

int main(int argc, char** argv)
{
	pid_t pid = -1;
	configurationBase_t *conf_base = NULL;

	//user directory create ++
	while(1) {
		if(tools_check_exist_file(USER_DATA_DIR, 2) >= 0) {
			break;
		}
		printf("User dir : %s can't find\n");
		system(MKDIR_USER_DIR);
		sleep(1);
	}
	//user directory create --

	stackdump_init();
	logd_init();

	LOGI(LOG_TARGET, "Build date:" __DATE__ __TIME__ "\n");
	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if((pid = fork()) < 0) {
		error_critical(eERROR_EXIT, "fork Error");
	} else if(pid != 0) {
		exit(0);
	}
	
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	setsid();

	if(sender_init() < 0)
	{
		LOGE(LOG_TARGET, "sender_init  Error\n");
		error_critical(eERROR_EXIT, "sender_init Error");
	}

	//at_open(e_DEV_TX501_BASE, NULL, NULL, NULL);
	//at_open(e_DEV_TX501_BASE, NULL, NULL, "console");
	at_open(AT_LIB_TARGET_DEV, gtrack_at_noti_proc, gtrack_at_sms_proc, "console");
	

	//at_start();

	//_initial_btn_check();

	//led_noti(eLedcmd_SYSTEM_INIT);
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L) 
	_gpio_check();
#endif

	error_rm_no_mon();

	if(argc == 2 && strcmp(argv[1], "-g") == 0)
	{
	#ifdef USE_GPS_MODEL
		if(gps_set_pipe_for_emul() >= 0)
			LOGT(LOG_TARGET, "Enable GPS Emulation!\n");
	#endif
	}

	if((load_config_base()) == NULL) {
		LOGE(LOG_TARGET, "load_config_base Error\n");
		error_critical(eERROR_LOG, "load_config_base Error");
		load_config_base_default();
	}

	if(load_config_model() == NULL) {
		LOGE(LOG_TARGET, "load_config_model Error\n");
		error_critical(eERROR_LOG, "load_config_model Error");
		load_config_model_default();
	}

	if(load_config_user() == NULL) {
		LOGE(LOG_TARGET, "load_config_user Error\n");
		error_critical(eERROR_LOG, "load_config_user Error");
		load_config_user_default();
	}

	conf_base = get_config_base();

//	_power_check();
	
	_set_tz();

	dmmgr_init();

	crit_init();

#ifdef USE_GPS_MODEL
	mileage_read();

	gps_valid_data_read();

	// agps setting for tl500
	at_set_gps_on(e_GPS_ON_TYPE_SET_ENV_AGPS, e_GPS_BOOT_TYPE_NULL);
#endif

#ifdef USE_GPS_FAKE
   // mileage_read();
    gps_valid_data_read();
#endif

	if(battery_init_adc() < 0)
	{
		LOGE(LOG_TARGET, "battery_init_adc Error\n");
		error_critical(eERROR_EXIT, "battery_init_adc Error");
	}

    init_model_callback();
    // kksworks : force setting for iot KT cert.
#ifdef KT_FOTA_TEST_SVR
	thread_network_set_warn_timeout(0); // waring msg disable..
    set_auto_ota_tl500(0); // auto ota disable.
    set_max_network_fail_reset_cnt(0); // network invalid chk disable..
#endif

	stackdump_abort_base_callback = _deinit_essential_functions;

/*
#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)

	int stat_num = 0;
	char buf[512];
	if(at_get_state(&stat_num, buf, sizeof(buf)) >= 0) 
	{
		printf("stat_num = [%d]\n", stat_num);
		if(stat_num == 0) 
		{
			if(!strncmp(buf, "OPEN", 4)) //not service available
			{
				wcdma_error_led_notification();
			}
		}
		else if(stat_num == 14) //usim not detect
		{
			wcdma_error_led_notification();
		}
	}
#endif
*/

//	at_set_alive(conf_base->common.alive_time_sec);
//	_get_essential_data_blocking();

	watchdog_set_cur_ktime(eWdMain);
	watchdog_process();

	int i = 0;
	pthread_attr_t attr;

	for(i = 0; i < MAX_THREAD_COUNT; i++) {
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, threads[i].stack_size);
		
		if (threads[i].thread == NULL)
			break;
			
		if(pthread_create(&tid[i], &attr, threads[i].thread, NULL) != 0) {
			LOGE(LOG_TARGET, "Create Thread Fail.. exit Program...\r\n");
			error_critical(eERROR_EXIT, "pthread_create Error");
		}

		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();
		
		usleep(100000);
	}

	// model thread..
	if(thread_model != NULL)
	{
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 256*1024);
		if(pthread_create(&tid_model, &attr, (void *)thread_model, NULL) != 0) {
			LOGE(LOG_TARGET, "Create Thread Fail.. exit Program...\r\n");
			error_critical(eERROR_EXIT, "pthread_create Error");
		}
	}

	LOGT(LOG_TARGET, "INIT Complete.\n");

	watchdog_set_init_ktime(eWdMain);

#ifdef DTG_ENABLE 
	tacom_init();
	tacoc_run();
#endif

	main_loop_callback();

	_deinit_resource();
	
	return 0;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
