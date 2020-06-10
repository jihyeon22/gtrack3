<<<<<<< HEAD
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <base/config.h>
#include <base/sender.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include <pthread.h>

#include "callback.h"
#include "config.h"
#include "custom.h"
#include "data-list.h"
#include "debug.h"
#include "netcom.h"

#include "thread-keypad.h"
#include <disc_dtg_pkt.h>

#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/hdlc_async.h>
#include <mdt800/file_mileage.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <tacom/tacom_std_protocol.h>

#include "dtg_pkt_senario.h"
#include "mdt_pkt_senario.h"
#include "dvr_pkt_senario.h"

#include <mdsapi/mds_api.h>

pthread_t tid_dvr_thread = NULL;
static int _g_run_dvr_thread_run = 0;

// --------------------------------------------------
// make and send pkt..
// --------------------------------------------------
int bizincar_dvr__send_pkt(char* dvr_str, int str_len)
{
    bizincar_dvr__dev_data_t dvr_pkt ={0,};
    char temp_buff[1024] = {0,};
    int dvr_pkt_len = 0;

    if ( str_len > 3 )
    {
        memcpy(&temp_buff, dvr_str, str_len);
        dvr_pkt_len = sprintf(dvr_pkt.buff, ">%s<", temp_buff);
        dvr_pkt.buff_len = dvr_pkt_len;

        sender_add_data_to_buffer(eDVR_CUSTOM_EVT__DVR_REPORT, &dvr_pkt, ePIPE_1);
    }

    return 0;
}

// -----------------------------------------------------
// make pkt call from netcom.
// -----------------------------------------------------
int bizincar_dvr__make_evt_pkt(unsigned char **pbuf, unsigned short *packet_len, char* record, int rec_len )
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	lotte_packet2_t packet;

	gps_get_curr_data(&gpsdata);

	if ( ( gpsdata.active != eACTIVE ) || (gpsdata.lat == 0 ) || (gpsdata.lon == 0 ) ) 
	{
		gpsData_t last_gpsdata;
		gps_valid_data_get(&last_gpsdata);
		gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;
	}
	
    if ( gpsdata.year < 2016)
    {
//        configurationBase_t *conf = get_config_base();
        struct tm loc_time;
//        gpsdata.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
//       _gps_utc_sec_localtime(gpsdata.utc_sec, &loc_time, conf->gps.gps_time_zone);
        gpsdata.year = loc_time.tm_year + 1900;
        gpsdata.mon = loc_time.tm_mon + 1;
        gpsdata.day = loc_time.tm_mday;
        gpsdata.hour = loc_time.tm_hour;
        gpsdata.min = loc_time.tm_min;
        gpsdata.sec = loc_time.tm_sec;
    }

	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	create_report2_data(DVR_EVT_CODE, &packet, gpsdata, record, rec_len);

    packet.vehicle_odo = bizincar_dtg__vehicle_odo_diff_dvr();

	crc = crc8(crc, (unsigned char *)&packet, sizeof(lotte_packet2_t));
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, sizeof(lotte_packet2_t));
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;
	
	return 0;
}


int bizincar_dvr__parse_resp(bizincar_dvr__server_resp_t* resp)
{
    int resp_code = -1;
    int ret_val = -1;

    printf("%s() -> [%d]\r\n", __func__, __LINE__);
    resp_code = resp->packet_ret_code;

    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    
    switch (resp_code)
    {
        case 0 : 
            ret_val = -1;
            break;
        case 1 : // success
            ret_val = 0;
            break;
        case 2 : // success
            ret_val = 0;
            break;
        default : 
            ret_val = -1;
            break;
    }   

    printf("%s() -> [%d]\r\n", __func__, __LINE__);

    return ret_val;
}

// -------------------
// uart tools..
// -------------------
int g_dvr_fd = DVR_INVAILD_FD;

#define DEBUG_MSG_DVR_UART

static char g_dvr_dev_path_str[64] = {0,};
static int g_dvr_dev_baudrate = 0;

static int _init_dvr_uart(char* dev, int baud , int *fd)
{
	struct termios newtio;
	*fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if(*fd < 0) {
		printf("%s> uart dev '%s' open fail [%d]\n", __func__, dev, *fd);
		return -1;
	}

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts

	switch(baud)
	{
		case 115200 :
			newtio.c_cflag |= B115200;
			break;
		case 57600 :
			newtio.c_cflag |= B57600;
			break;
		case 38400 :
			newtio.c_cflag |= B38400;
			break;
		case 19200 :
			newtio.c_cflag |= B19200;
			break;
		case 9600 :
			newtio.c_cflag |= B9600;
			break;
		case 4800 :
			newtio.c_cflag |= B4800;
			break;
		case 2400 :
			newtio.c_cflag |= B2400;
			break;
		default :
			newtio.c_cflag |= B115200;
			break;
	}

	newtio.c_lflag = 0;
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1?? ????
	//newtio.c_cc[VMIN] = vmin; // ??? n ???? ???? ?????? ????
	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 0;
	tcflush(*fd, TCIFLUSH);
	tcsetattr(*fd, TCSANOW, &newtio);
    
	return 0;
}

int dvr__uart_set_dev(char* dev_name)
{
    if ( ( dev_name != NULL ) && ( strlen(dev_name) > 0 ) )
        strcpy(g_dvr_dev_path_str, dev_name);
    else
        strcpy(g_dvr_dev_path_str, DVR_DEV_DEFAULT_PATH);

    return 0;
}

int dvr__uart_set_baudrate(int baudrate)
{
    if ( baudrate > 0 )
        g_dvr_dev_baudrate = baudrate;
    else
        g_dvr_dev_baudrate = DVR_DEV_DEFAULT_BAUDRATE;
    
    return 0;
}


int dvr__uart_init()
{
	int ret = 0;
	
	if ( g_dvr_fd <= 0 )
	{
		ret = _init_dvr_uart(DVR_DEV_DEFAULT_PATH, DVR_DEV_DEFAULT_BAUDRATE, &g_dvr_fd);
	}
	else
	{
		return DVR_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_dvr_fd = DVR_INVAILD_FD;
		return DVR_RET_FAIL;
	}
	
	printf("init dvr uart [%d], [%d]\r\n",ret, g_dvr_fd);
	
	return DVR_RET_SUCCESS;
}


int dvr__uart_deinit()
{
	if (g_dvr_fd > 0)
		close(g_dvr_fd);
	
	g_dvr_fd = DVR_INVAILD_FD;
	return 0;
}

int _uart_tool__uart_get_fd()
{
	return g_dvr_fd;
}


int dvr__uart_init_stat()
{
	if ( g_dvr_fd <= 0)
	{
		return DVR_RET_FAIL;
	}
	else
	{
		return DVR_RET_SUCCESS;
	}
}


int _uart_tool__uart_chk()
{
    int i = 0;
    
	for(i = 0; i < DVR_UART_INIT_TRY_CNT ; i++)
	{
		if (dvr__uart_init() == DVR_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (dvr__uart_init_stat() == DVR_RET_FAIL)
	{
		return DVR_CMD_UART_INIT_FAIL;
	}

    return DVR_RET_SUCCESS;
}

int dvr__uart_wait_read(int fd, char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;

	int read_cnt = 0;
	
	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return -1;
		
		read_cnt = read(fd, buf, buf_len);
		
		if ( read_cnt <= 0)
			return -1;

		break; //success
	}

	return read_cnt;
}



void dvr__mgr_read_thread(void)
{
    int  dvr_fd = -1;
    int  uart_ret = 0;
    char dvr_recv_data[MAX_DVR_RET_BUFF_SIZE] ={0,};

    static int  to_line_read = 0;
    char* p_tmp_buff = NULL;

    static int read_fail_cnt = 0;

    while(_g_run_dvr_thread_run)
    {
    //    usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_DRV_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[dvr read thread] run..\r\n");
        if ( _uart_tool__uart_chk() != DVR_RET_SUCCESS )
        {
            printf("[dvr read thread] dvr init fail..\r\n");
            sleep(1);
            continue;
        }

        dvr_fd = _uart_tool__uart_get_fd();

        if ( dvr_fd == DVR_INVAILD_FD )
        {
            printf("[dvr read thread] get dvr fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(dvr_recv_data, 0x00, sizeof(dvr_recv_data));

        uart_ret =  mds_api_uart_read(dvr_fd, (void*)dvr_recv_data,  sizeof(dvr_recv_data), DVR_UART_READ_THREAD_TIMEOUT);

        if ( uart_ret <= 0 )
        {
            //printf("[dvr read thread] read fail case 1 do anything...\r\n");
            read_fail_cnt++;
            continue;
        }

 
        //printf("---------------------------------------------------------------------\r\n");
        //debug_hexdump_buff(dvr_recv_data, uart_ret);
        //printf("[dvr read thread] read success [%s] \r\n", dvr_recv_data);
        //printf("---------------------------------------------------------------------\r\n");

        read_fail_cnt = 0;
        to_line_read = strlen(dvr_recv_data);
        p_tmp_buff = dvr_recv_data;

        while( to_line_read > 0 )
        {
            char read_line_buff[MAX_DVR_RET_BUFF_SIZE] = {0,};
            int  read_line_len = 0;

            // 모든데이터는 1 line 이 한개 데이터다.
            // 여러개 line 이 한번에 들어올때가 있어서 버퍼를 읽으면서 proc 에 던진다.
            if ( ( read_line_len = mds_api_read_line(p_tmp_buff, to_line_read, read_line_buff, sizeof(read_line_buff)) ) <= 0 )
                break;
                 
            to_line_read -= read_line_len;
            p_tmp_buff += read_line_len;

            if ( read_line_len > 3 )
            {
                printf("---------------------------------------------------------------------\r\n");
                printf("[dvr read thread] read line success [%s] \r\n", read_line_buff);
                printf("---------------------------------------------------------------------\r\n");

                bizincar_dvr__send_pkt(read_line_buff, read_line_len);
            }
        }

    }

}

int bizincar_dvr__mgr_init()
{
    pthread_attr_t attr;
    _g_run_dvr_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_dvr_thread == NULL )
        pthread_create(&tid_dvr_thread, &attr, dvr__mgr_read_thread, NULL);

    return 0;
}

int dvr__mgr_thread_stop()
{
    _g_run_dvr_thread_run = 0;
    return 0;
}


#if 0


=======
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <base/config.h>
#include <base/sender.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include <pthread.h>

#include "callback.h"
#include "config.h"
#include "custom.h"
#include "data-list.h"
#include "debug.h"
#include "netcom.h"

#include "thread-keypad.h"
#include <disc_dtg_pkt.h>

#include <mdt800/packet.h>
#include <mdt800/gpsmng.h>
#include <mdt800/hdlc_async.h>
#include <mdt800/file_mileage.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <tacom/tacom_std_protocol.h>

#include "dtg_pkt_senario.h"
#include "mdt_pkt_senario.h"
#include "dvr_pkt_senario.h"

#include <mdsapi/mds_api.h>

pthread_t tid_dvr_thread = NULL;
static int _g_run_dvr_thread_run = 0;

// --------------------------------------------------
// make and send pkt..
// --------------------------------------------------
int bizincar_dvr__send_pkt(char* dvr_str, int str_len)
{
    bizincar_dvr__dev_data_t dvr_pkt ={0,};
    char temp_buff[1024] = {0,};
    int dvr_pkt_len = 0;

    if ( str_len > 3 )
    {
        memcpy(&temp_buff, dvr_str, str_len);
        dvr_pkt_len = sprintf(dvr_pkt.buff, ">%s<", temp_buff);
        dvr_pkt.buff_len = dvr_pkt_len;

        sender_add_data_to_buffer(eDVR_CUSTOM_EVT__DVR_REPORT, &dvr_pkt, ePIPE_1);
    }

    return 0;
}

// -----------------------------------------------------
// make pkt call from netcom.
// -----------------------------------------------------
int bizincar_dvr__make_evt_pkt(unsigned char **pbuf, unsigned short *packet_len, char* record, int rec_len )
{
	unsigned short crc = 0;
	int enclen = 0;
	unsigned char *p_encbuf;
	gpsData_t gpsdata;
	lotte_packet2_t packet;

	gps_get_curr_data(&gpsdata);

	if ( ( gpsdata.active != eACTIVE ) || (gpsdata.lat == 0 ) || (gpsdata.lon == 0 ) ) 
	{
		gpsData_t last_gpsdata;
		gps_valid_data_get(&last_gpsdata);
		gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;
	}
	
    if ( gpsdata.year < 2016)
    {
//        configurationBase_t *conf = get_config_base();
        struct tm loc_time;
//        gpsdata.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
//       _gps_utc_sec_localtime(gpsdata.utc_sec, &loc_time, conf->gps.gps_time_zone);
        gpsdata.year = loc_time.tm_year + 1900;
        gpsdata.mon = loc_time.tm_mon + 1;
        gpsdata.day = loc_time.tm_mday;
        gpsdata.hour = loc_time.tm_hour;
        gpsdata.min = loc_time.tm_min;
        gpsdata.sec = loc_time.tm_sec;
    }

	if(create_report2_divert_buffer(&p_encbuf, 1) < 0)
	{
		LOGE(LOG_TARGET, "%s> create report divert buffer fail\n", __func__);
		return -1;
	}
	
	create_report2_data(DVR_EVT_CODE, &packet, gpsdata, record, rec_len);

    packet.vehicle_odo = bizincar_dtg__vehicle_odo_diff_dvr();

	crc = crc8(crc, (unsigned char *)&packet, sizeof(lotte_packet2_t));
	enclen = hdlc_async_encode(p_encbuf, (unsigned char *)&packet, sizeof(lotte_packet2_t));
	enclen += hdlc_async_encode(&p_encbuf[enclen], (unsigned char *)&crc, sizeof(crc));
	p_encbuf[enclen++] = MDT800_PACKET_END_FLAG;

	*packet_len = enclen;
	*pbuf = p_encbuf;
	
	return 0;
}


int bizincar_dvr__parse_resp(bizincar_dvr__server_resp_t* resp)
{
    int resp_code = -1;
    int ret_val = -1;

    printf("%s() -> [%d]\r\n", __func__, __LINE__);
    resp_code = resp->packet_ret_code;

    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    LOGI(LOG_TARGET, "%s > retcode [%d]\n", __func__, resp_code);
    
    switch (resp_code)
    {
        case 0 : 
            ret_val = -1;
            break;
        case 1 : // success
            ret_val = 0;
            break;
        case 2 : // success
            ret_val = 0;
            break;
        default : 
            ret_val = -1;
            break;
    }   

    printf("%s() -> [%d]\r\n", __func__, __LINE__);

    return ret_val;
}

// -------------------
// uart tools..
// -------------------
int g_dvr_fd = DVR_INVAILD_FD;

#define DEBUG_MSG_DVR_UART

static char g_dvr_dev_path_str[64] = {0,};
static int g_dvr_dev_baudrate = 0;

static int _init_dvr_uart(char* dev, int baud , int *fd)
{
	struct termios newtio;
	*fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if(*fd < 0) {
		printf("%s> uart dev '%s' open fail [%d]\n", __func__, dev, *fd);
		return -1;
	}

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts

	switch(baud)
	{
		case 115200 :
			newtio.c_cflag |= B115200;
			break;
		case 57600 :
			newtio.c_cflag |= B57600;
			break;
		case 38400 :
			newtio.c_cflag |= B38400;
			break;
		case 19200 :
			newtio.c_cflag |= B19200;
			break;
		case 9600 :
			newtio.c_cflag |= B9600;
			break;
		case 4800 :
			newtio.c_cflag |= B4800;
			break;
		case 2400 :
			newtio.c_cflag |= B2400;
			break;
		default :
			newtio.c_cflag |= B115200;
			break;
	}

	newtio.c_lflag = 0;
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1?? ????
	//newtio.c_cc[VMIN] = vmin; // ??? n ???? ???? ?????? ????
	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 0;
	tcflush(*fd, TCIFLUSH);
	tcsetattr(*fd, TCSANOW, &newtio);
    
	return 0;
}

int dvr__uart_set_dev(char* dev_name)
{
    if ( ( dev_name != NULL ) && ( strlen(dev_name) > 0 ) )
        strcpy(g_dvr_dev_path_str, dev_name);
    else
        strcpy(g_dvr_dev_path_str, DVR_DEV_DEFAULT_PATH);

    return 0;
}

int dvr__uart_set_baudrate(int baudrate)
{
    if ( baudrate > 0 )
        g_dvr_dev_baudrate = baudrate;
    else
        g_dvr_dev_baudrate = DVR_DEV_DEFAULT_BAUDRATE;
    
    return 0;
}


int dvr__uart_init()
{
	int ret = 0;
	
	if ( g_dvr_fd <= 0 )
	{
		ret = _init_dvr_uart(DVR_DEV_DEFAULT_PATH, DVR_DEV_DEFAULT_BAUDRATE, &g_dvr_fd);
	}
	else
	{
		return DVR_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_dvr_fd = DVR_INVAILD_FD;
		return DVR_RET_FAIL;
	}
	
	printf("init dvr uart [%d], [%d]\r\n",ret, g_dvr_fd);
	
	return DVR_RET_SUCCESS;
}


int dvr__uart_deinit()
{
	if (g_dvr_fd > 0)
		close(g_dvr_fd);
	
	g_dvr_fd = DVR_INVAILD_FD;
	return 0;
}

int _uart_tool__uart_get_fd()
{
	return g_dvr_fd;
}


int dvr__uart_init_stat()
{
	if ( g_dvr_fd <= 0)
	{
		return DVR_RET_FAIL;
	}
	else
	{
		return DVR_RET_SUCCESS;
	}
}


int _uart_tool__uart_chk()
{
    int i = 0;
    
	for(i = 0; i < DVR_UART_INIT_TRY_CNT ; i++)
	{
		if (dvr__uart_init() == DVR_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (dvr__uart_init_stat() == DVR_RET_FAIL)
	{
		return DVR_CMD_UART_INIT_FAIL;
	}

    return DVR_RET_SUCCESS;
}

int dvr__uart_wait_read(int fd, char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;

	int read_cnt = 0;
	
	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return -1;
		
		read_cnt = read(fd, buf, buf_len);
		
		if ( read_cnt <= 0)
			return -1;

		break; //success
	}

	return read_cnt;
}



void dvr__mgr_read_thread(void)
{
    int  dvr_fd = -1;
    int  uart_ret = 0;
    char dvr_recv_data[MAX_DVR_RET_BUFF_SIZE] ={0,};

    static int  to_line_read = 0;
    char* p_tmp_buff = NULL;

    static int read_fail_cnt = 0;

    while(_g_run_dvr_thread_run)
    {
    //    usleep(300); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함

        if ( read_fail_cnt > MAX_DRV_UART_INVAILD_CHK )
        {
            read_fail_cnt = 0;
        }

        // printf("[dvr read thread] run..\r\n");
        if ( _uart_tool__uart_chk() != DVR_RET_SUCCESS )
        {
            printf("[dvr read thread] dvr init fail..\r\n");
            sleep(1);
            continue;
        }

        dvr_fd = _uart_tool__uart_get_fd();

        if ( dvr_fd == DVR_INVAILD_FD )
        {
            printf("[dvr read thread] get dvr fd fail\r\n");
            sleep(1);
            continue;
        }

        memset(dvr_recv_data, 0x00, sizeof(dvr_recv_data));

        uart_ret =  mds_api_uart_read(dvr_fd, (void*)dvr_recv_data,  sizeof(dvr_recv_data), DVR_UART_READ_THREAD_TIMEOUT);

        if ( uart_ret <= 0 )
        {
            //printf("[dvr read thread] read fail case 1 do anything...\r\n");
            read_fail_cnt++;
            continue;
        }

 
        //printf("---------------------------------------------------------------------\r\n");
        //debug_hexdump_buff(dvr_recv_data, uart_ret);
        //printf("[dvr read thread] read success [%s] \r\n", dvr_recv_data);
        //printf("---------------------------------------------------------------------\r\n");

        read_fail_cnt = 0;
        to_line_read = strlen(dvr_recv_data);
        p_tmp_buff = dvr_recv_data;

        while( to_line_read > 0 )
        {
            char read_line_buff[MAX_DVR_RET_BUFF_SIZE] = {0,};
            int  read_line_len = 0;

            // 모든데이터는 1 line 이 한개 데이터다.
            // 여러개 line 이 한번에 들어올때가 있어서 버퍼를 읽으면서 proc 에 던진다.
            if ( ( read_line_len = mds_api_read_line(p_tmp_buff, to_line_read, read_line_buff, sizeof(read_line_buff)) ) <= 0 )
                break;
                 
            to_line_read -= read_line_len;
            p_tmp_buff += read_line_len;

            if ( read_line_len > 3 )
            {
                printf("---------------------------------------------------------------------\r\n");
                printf("[dvr read thread] read line success [%s] \r\n", read_line_buff);
                printf("---------------------------------------------------------------------\r\n");

                bizincar_dvr__send_pkt(read_line_buff, read_line_len);
            }
        }

    }

}

int bizincar_dvr__mgr_init()
{
    pthread_attr_t attr;
    _g_run_dvr_thread_run = 1;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);

    if ( tid_dvr_thread == NULL )
        pthread_create(&tid_dvr_thread, &attr, dvr__mgr_read_thread, NULL);

    return 0;
}

int dvr__mgr_thread_stop()
{
    _g_run_dvr_thread_run = 0;
    return 0;
}


#if 0


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
#endif