/**
 *       @file  tacom_sinhung.c
 *      @brief  tacom sinhung model
 *
 * Detailed description starts here.
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013??03??14?? *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technology, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include <stdio.h>
#include <math.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <wrapper/dtg_log.h>

#include <tacom/tacom_internal.h>
#include <tacom/tacom_protocol.h>
#include <mdsapi/mds_api.h>

int send_cmd(char *cmd, struct tacom_setup *tm_setup, char *caller_detector)
{
	if(caller_detector != NULL)
		DTG_LOGD("taco cmd : %s from %s +++", cmd, caller_detector);
	else
		DTG_LOGD("taco cmd : %s +++", cmd);

	int idx = 0;
	char cmdstr[32] = {0};
	int cmdlen;
	int result;
	unsigned short crc16_value;
	unsigned char *crc_data;

	/* command check */
	if (!cmd)
		return -1;
	else
		cmdlen = strlen(cmd);
	DTG_LOGD("%s: %s: cmdlen : %d", __FILE__, __func__, cmdlen);
	
	/* Uart flush
	 * 2초동???�이?��? ?�으��?flush?�걸��?간주?�다
	 */

#if defined(DEVICE_MODEL_UCAR)
	DTG_LOGD("%s : UCAR DTG FLUSH SKIP AS BROADCAST WAY", __func__);
#elif defined(DEVICE_MODEL_DAESIN)
	DTG_LOGD("%s : DEASIN DTGH FLUSH SKIP AS BROADCAST WAY", __func__);
#elif defined(DEVICE_MODEL_CJ)
	mds_api_uart_flush(dtg_uart_fd, 5, 0); // TODO: UART API FIX
#else 
	mds_api_uart_flush(dtg_uart_fd, 2, 0);  // TODO: UART API FIX
#endif

	/* cmd ?�성 */
	DTG_LOGD("%s : cmd processing!!", __func__);
	cmdstr[idx++] = tm_setup->start_mark;
	
	memcpy(cmdstr + idx, cmd, cmdlen);
	idx += cmdlen;
	if (tm_setup->conf_flag & (0x1 << TACOM_CRC16_BIT)) {
		crc16_value = mds_api_crc16_get((unsigned char*)cmd, cmdlen);	//  TODO: API FIX
		crc_data = (unsigned char *)&crc16_value;
		cmdstr[idx++] = crc_data[(tm_setup->conf_flag >> TACOM_CRC_ENDIAN1_BIT) & 0x1];
		cmdstr[idx++] = crc_data[(tm_setup->conf_flag >> TACOM_CRC_ENDIAN0_BIT) & 0x1];
	}
	cmdstr[idx++] = tm_setup->end_mark;

	result = mds_api_uart_write(dtg_uart_fd, cmdstr, idx);  // TODO: UART API FIX
	if (result != idx) {
		DTG_LOGE("%s: %s: mds_api_uart_write fail", __FILE__, __func__);
		return -1;
	}
	return idx;
}

int recv_data(char *data, int len, char eoc, int b_size)
{
	DTG_LOGD("%s : %s +++", __FILE__, __func__);

	char *buf = NULL;
	int ret;
	int bytes = 0;
	int readcnt = 0;
	int remaining = len;

	buf = malloc(b_size);
	if(buf == NULL)
		return -1;
	
	while (remaining > 0) 
	{
		/**
		 * read
		 */
		memset(buf, 0, b_size);
		remaining -= readcnt;
		if (remaining < b_size)
			bytes = remaining;
		else 
			bytes = b_size;
		
		readcnt = mds_api_uart_read(dtg_uart_fd, (unsigned char *)buf, bytes, 7); //  TODO: API FIX
		/* Failure */
		if (readcnt < 0) {
			DTG_LOGE("%s: %s: Uart Read Fail", __FILE__, __func__);
			ret = -1;
			goto failed;
		} else if (readcnt == 0) {		/* Timeout */
			if (remaining == len) { 	// ?�이?��? ?��? ?�어?��? ?�을 ??				DTG_LOGE("%s: %s: No Data!!", __FILE__, __func__);
				ret = -1;
				goto failed;
			} else {					// 중간??Uart data가 ?�어?��? ?�을 ??				LOGW("%s: %s: EOC(%02x) is not found!!", __FILE__, __func__, eoc);
				break;
			}
		} else if (readcnt > 0) {	/* Success */
			if (eoc) {			/* eoc(end of char) check */
				/* eoc가 ?�어?�면 uart read ?�행??종료?�다. */
				if (buf[readcnt - 1] == eoc) {
					DTG_LOGD("%s: %s: EOC(%02x) is found!!", __FILE__, __func__, eoc);
					memcpy(data + (len - remaining), buf, readcnt);
					remaining -= readcnt;
					break;
				}
			}
			memcpy(data + (len - remaining), buf, readcnt);
		}

#if defined(DEVICE_MODEL_CJ) || defined(DEVICE_MODEL_LOOP)
 
			if (len - remaining > 1024) {
				DTG_LOGD("%s: %s: readed %d Kb", __FILE__, __func__, (len-remaining) / 1024);
			} else {
				DTG_LOGD("%s: %s: readed %d bytes", __FILE__, __func__, (len-remaining));
			}
#endif
	}

	if (len - remaining > 1024) {
		DTG_LOGD("%s: %s: readed %d Kb", __FILE__, __func__, (len-remaining) / 1024);
	} else {
		DTG_LOGD("%s: %s: readed %d bytes", __FILE__, __func__, (len-remaining));
	}

	ret = len - remaining;

failed:
	if(buf != NULL)
		free(buf);

	return ret;
}

static int get_data_count(char *buf, int len)
{
	int i;
	int count = 0;
	int num = 0;
	
	for (i = len - 1; i >= 0; i--) {
		num += (buf[i] - '0') * pow(10, count);
		count++;
	}

	return num;
}

static int is_valid_data(int type, char *buf, int len, struct tacom_setup *tm_setup)
{
	int i;
	if (type == tm_setup->data_rl) {
		if (buf[0] != tm_setup->start_mark) return 0;

		for (i = 1; i < len - 1; i++)
			if (!(buf[i] >= '0' && buf[i] <='9'))
				return -1;

		if (buf[len - 1] != tm_setup->end_mark)
			return 0;
	}

	return 1;
}

int crc_check(unsigned char * buf, int r_size, struct tacom_setup *tm_setup)
{
	unsigned short crc16_value;
	unsigned char *crc_data;

	mds_api_crc16_get(NULL, 0);	//  TODO: API FIX
	crc16_value = mds_api_crc16_get(&buf[1], r_size - 4); // //  TODO: API FIX
	crc_data = (unsigned char *) &crc16_value;

	if (crc_data[(tm_setup->conf_flag >> TACOM_CRC_ENDIAN1_BIT) & 0x1] != buf[r_size - 3] || 
		crc_data[(tm_setup->conf_flag >> TACOM_CRC_ENDIAN0_BIT) & 0x1] != buf[r_size - 2])
	{
		DTG_LOGE("%sCRC ERROR", tm_setup->tacom_dev);
		DTG_LOGE("%sRECEIVED CRC = [0x%2x][0x%2x]", tm_setup->tacom_dev, 
			buf[r_size - 3], buf[r_size - 2]);
		DTG_LOGE("%sCALCULATED CRC = [0x%2x][0x%2x]", tm_setup->tacom_dev, 
			crc_data[(tm_setup->conf_flag >> TACOM_CRC_ENDIAN1_BIT) & 0x1], 
            crc_data[(tm_setup->conf_flag >> TACOM_CRC_ENDIAN0_BIT) & 0x1]);
		return -1;
	}
	return 0;
}

int unreaded_records_num()
{
	char buf[128] = { 0 };
	int r_size;
	int num = 0;
	//unsigned short crc16_value;
	//unsigned char *crc_data;

	TACOM *tm = tacom_get_cur_context();

	/* Send RL Command */
	if (tm->tm_ops->tm_send_cmd(tm->tm_setup->cmd_rl, tm->tm_setup, __func__) < 0) {
		DTG_LOGE("%sRL ERROR", tm->tm_setup->tacom_dev);
		return -1;
	}

	/* Recv Data */
	r_size = tm->tm_ops->tm_recv_data(buf, sizeof(buf), tm->tm_setup->end_mark);
	if (r_size < 0) {
		return -1;
	}

	if (tm->tm_setup->conf_flag & (0x1 << TACOM_CRC16_BIT)) {
		if (crc_check((unsigned char *)buf, r_size, tm->tm_setup) < 0)
			return -1;
	} else {
		if (!is_valid_data(tm->tm_setup->cmd_rl, buf, r_size, tm->tm_setup))
			return -1;
	}

	r_size -= ((((tm->tm_setup->conf_flag >> TACOM_CRC16_BIT) & 0x1) * 2) + 2);
	num = get_data_count(&buf[1], r_size);
	DTG_LOGD("unreaded_records_num : [%d]\n", num);
	return num;
}

int read_current()
{
    DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
    int bytes, result;
    char buf[1024*64] = {0};
    
	TACOM *tm = tacom_get_cur_context();

    /* Send RR Command */
    if (tm->tm_ops->tm_send_cmd(tm->tm_setup->cmd_rc, tm->tm_setup, __func__) < 0) {
        DTG_LOGE("%sRC ERROR1", tm->tm_setup->tacom_dev);
        return -1;
    }
    
    memset(buf, 0, 1024*64);
    bytes = tm->tm_ops->tm_recv_data(buf, 1024*64, 0x00);
    if (bytes < 0) {
        DTG_LOGE("%sRC ERROR2", tm->tm_setup->tacom_dev);
        return -1;
    }

	if (tm->tm_setup->conf_flag & (0x1 << TACOM_CRC16_BIT)) {
		if (bytes > 0) {
			if (crc_check((unsigned char *)buf, bytes, tm->tm_setup) < 0)
				return -1;
		} else {
			DTG_LOGE("%sDATA INVALIDATE ERROR", tm->tm_setup->tacom_dev);
			return -1;
		}
	} else {
		DTG_LOGE("[%s : %d ]Cannot CRC16 Check", __func__, __LINE__);
	}

	result = tm->tm_ops->tm_current_record_parsing(buf, 
						bytes, tm->tm_strm.stream);
	if (result < 0) {
        DTG_LOGE("%sRC ERROR3", tm->tm_setup->tacom_dev);
		return -1;
	}

    return result;
}

int ack_records (int read_num)
{
	int result;
	char recv_buf[128] = {0};

	TACOM *tm = tacom_get_cur_context();

	if (tm->tm_ops->tm_send_cmd(tm->tm_setup->cmd_rq, tm->tm_setup, __func__) < 0) {
		DTG_LOGE("%sRQ ERROR", tm->tm_setup->tacom_dev);
		return -1;
	}

	result = tm->tm_ops->tm_recv_data(recv_buf, sizeof(recv_buf), 
									tm->tm_setup->end_mark);
	if (result < 0) {
		DTG_LOGE("%sRQ RECV ERROR", tm->tm_setup->tacom_dev);
	}

	return result;
}


int custom_command(char *command, char *result_str, int result_str_len)
{
	unsigned char cust_resp[128];
	int r_size;
	int num = 0;
	//unsigned short crc16_value;
	//unsigned char *crc_data;
	memset(cust_resp, 0, 128);

	TACOM *tm = tacom_get_cur_context();

	/* Send custom Command */
	if (tm->tm_ops->tm_send_cmd(command, tm->tm_setup, __func__) < 0) {
		DTG_LOGE("%s %s ERROR", tm->tm_setup->tacom_dev, command);
		strcpy(result_str, "Fail Set Command err:#1"); //send error
		return 0;
	}

	/* Recv Data */
	r_size = tm->tm_ops->tm_recv_data(cust_resp, sizeof(cust_resp), tm->tm_setup->end_mark);
	if (r_size < 0) {
		strcpy(result_str, "Fail Set Command err:#2"); //recv error
		return 0;
	}

	if (tm->tm_setup->conf_flag & (0x1 << TACOM_CRC16_BIT)) {
		if (crc_check((unsigned char *)cust_resp, r_size, tm->tm_setup) < 0) {
			strcpy(result_str, "Fail Set Command err:#3"); //crc error
			return 0;
		}
	}

	memset(result_str, 0, result_str_len);
	memcpy(result_str, &cust_resp[1], r_size-4);
	return 0;
}

