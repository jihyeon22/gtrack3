/**
 *       @file  tacom_simul.c
 *      @brief  tacom simulation model
 *
 * Detailed description starts here.
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013년 03월 11일
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technologt, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include <tacom_internal.h>
#include <tacom_protocol.h>

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#define SIMUL_SPEED 	20


static int simul_control(TACOM *tm, int type, char *data);
static int simul_unreaded_records_num (TACOM *tm);
static int simul_read_records (TACOM *tm, int r_num);
int simul_ack_records (TACOM *tm, int num);
int simul_read_last_records(TACOM *tm);
char * simul_get_info (TACOM *tm, int type);
int simul_get_info_hdr (TACOM *tm, void *infohdr);
int simul_set_info(TACOM *tm, int type, char *infodata);

static tacom_config_t simul_config;
static tacom_std_hdr_t simul_hdr;

typedef struct {
	tacom_std_data_t *elems;
	size_t size;
	unsigned int head;
	unsigned int tail;
} rb_t;

static const struct tm_ops simul_ops = {
	simul_control,
	simul_unreaded_records_num,
	simul_read_records,
	simul_ack_records,
	simul_read_last_records,
	simul_get_info,
	simul_get_info_hdr,
	simul_set_info
};

static rb_t *simul_rb;

static void rb_init (rb_t *rb, size_t size)
{
	rb->size = size;
	rb->head = 0;
	rb->tail = 0;
	rb->elems = (tacom_std_data_t *) calloc(rb->size, sizeof(tacom_std_data_t));
}

static void rb_free (rb_t *rb)
{
	free(rb);
}

static void rb_store(rb_t *rb, tacom_std_data_t data)
{
	rb->elems[rb->tail] = data;
	rb->tail = (rb->tail + 1) % rb->size;
	
	if (rb->tail == rb->head) {
		rb->head = (rb->head + 1) % rb->size;
	}
}

static int rb_peek(rb_t *rb)
{
//	return (rb->size + rb->head - rb->tail) % rb->size;
	return rb->tail - rb->head;
}

static int rb_is_empty(rb_t *rb)
{
	return rb->tail == rb->head;
}

static tacom_std_data_t rb_load(rb_t *rb)
{
	tacom_std_data_t data = rb->elems[rb->head];
	rb->head = (rb->head + 1) % rb->size;
	return data;
}

/**
 * TACOM SIMULATORATION DATA GENERATOR
 */
static struct timeval tv;
void generate_simul_data (int signo)
{
	int i;
	struct tm *t;
	char date_time[15] = {0};
	tacom_std_data_t dummy_data;

	memset(&dummy_data, '2', sizeof(dummy_data));:

	for (i = 0; i < SIMUL_SPEED; i++) {
		tv.tv_sec++;
		t = localtime(&tv.tv_sec);

		memset(date_time, 0, sizeof(date_time));
		sprintf(date_time, "%02d%02d%02d%02d%02d%02d%02d", 
				t->tm_year % 100, t->tm_mon + 1, t->tm_mday, 
				t->tm_hour, t->tm_min, t->tm_sec, 59);
		memcpy(dummy_data.date_time, date_time, 14);
		//	fprintf(stderr, "%02d:%02d:%02d:%02d:%02d:%02d.%02d 1sec data\n", 
		//			t->tm_year % 100, t->tm_mon + 1, t->tm_mday, 
		//			t->tm_hour, t->tm_min, t->tm_sec, 59);

		rb_store(simul_rb, dummy_data);
	}

	alarm(1);
}


int
simul_init (TACOM *tm)
{
	int i;
	size_t strm_max_size = 3100; // 3100 records
	char date_time[15] = {0};
	tacom_std_data_t simul_data;
	struct tm *t;

	tm->tm_ops = &simul_ops;
	simul_rb = (rb_t *) malloc(sizeof(rb_t));

	memset(&simul_data, '#', sizeof(simul_data));
	
	rb_init(simul_rb, 100000);

	gettimeofday(&tv, NULL);
	for (i = 0; i < 20000; i++) {
		tv.tv_sec++;
		t = localtime(&tv.tv_sec);
		
		sprintf(date_time, "%02d%02d%02d%02d%02d%02d%02d", 
				t->tm_year % 100, t->tm_mon + 1, t->tm_mday, 
				t->tm_hour, t->tm_min, t->tm_sec, 59);
		memcpy(simul_data.date_time, date_time, 14);
		rb_store(simul_rb, simul_data);
	};

	/* 스트림 버퍼 할당 */
	tm->tm_strm.size = strm_max_size;	// NeoM2M by TACO STD DATA record
	tm->tm_strm.stream = (tacom_std_data_t *) malloc(sizeof(tacom_std_data_t) * strm_max_size);
	
	memset(&simul_config, '#', sizeof(simul_config));
	memcpy(simul_config.vehicle_model, "Soul", strlen("Soul"));
	memcpy(simul_config.vehicle_id_num, "경기58나5858", strlen("경기58나5858"));
	memcpy(simul_config.vehicle_type, "11", 2);
	memcpy(simul_config.registration_num, "201006026123", 12);
	memcpy(simul_config.business_license_num, "1234567890", 10);
	memcpy(simul_config.driver_code, "234-567-891-123-45", 18);
	memcpy(simul_config.speed_compensation, "1234", 4);
	memcpy(simul_config.mileage_compensation, "1234", 4);
	memcpy(simul_config.fuel_compensation, "1234", 4);

	signal(SIGALRM, generate_simul_data);
	alarm(1);

	return 1;
}

int
simul_control(TACOM *_tm, int type, char *data)
{
	return 1;
}

int
simul_unreaded_records_num (TACOM *_tm)
{
	return rb_peek(simul_rb);
}

#define MAX_TACOM_RECORDS_PER_ONCE	3000
int
simul_read_records (TACOM *tm, int r_num)
{
	int strm_max_num = tm->tm_stream.size;
	tacom_std_data_t *data = tm->tm_stream.stream;
	int read_num = r_num;
	int i;

	memset(data, '#', sizeof(tacom_std_data_t) * strm_max_num);

	if (read_num > MAX_TACOM_RECORDS_PER_ONCE || read_num == 0)
		read_num = MAX_TACOM_RECORDS_PER_ONCE;

	for (i = 0; i < read_num; i++) {
		fprintf(stderr, "[%d]", i);
		if ((simul_rb->head + i) % simul_rb->size == simul_rb->tail) {
			fprintf(stderr, "\n");
			break;
		}
		*(data + i) = simul_rb->elems[simul_rb->head + i];
		usleep(10 * SIMUL_SPEED);
	}

	return i;
}

int
simul_ack_records (TACOM *__tm, int num)
{
	
	int i;
	for (i = 0; i < num; i++) {
		rb_load(simul_rb);
	}

	return 1;
}

int 
simul_read_last_records(TACOM *_tm)
{
	/* ???? */
	return -1;
}

char *
simul_get_info (TACOM *_tm, int type)
{
	switch (type) {
		case TACOM_INFO_VEHICLE_MODEL:
			printf("[SIMUL] tacom_get_info.. vehicle model : %s\n", simul_config.vehicle_model);
			return simul_config.vehicle_model;
		
		case TACOM_INFO_VEHICLE_ID_NUM:
			printf("[SIMUL] tacom_get_info.. vehicle id num : %s\n", simul_config.vehicle_id_num);
			return simul_config.vehicle_id_num;
		
		case TACOM_INFO_VEHICLE_TYPE:
			printf("[SIMUL] tacom_get_info.. vehicle type : %s\n", simul_config.vehicle_type);
			return simul_config.vehicle_type;
		
		case TACOM_INFO_REGISTRATION_NUM:
			printf("[SIMUL] tacom_get_info.. registration num : %s\n", simul_config.registration_num);
			return simul_config.registration_num;
		
		case TACOM_INFO_BUSINESS_LICENSE_NUM:
			printf("[SIMUL] tacom_get_info.. business license num  : %s\n", simul_config.business_license_num);
			return simul_config.business_license_num;
		
		case TACOM_INFO_DRIVER_CODE:
			printf("[SIMUL] tacom_get_info.. driver code  : %s\n", simul_config.driver_code);
			return simul_config.driver_code;
		
		case TACOM_INFO_SPEED_COMPENSATION:
			printf("[SIMUL] tacom_get_info.. speed compensation  : %s\n", simul_config.speed_compensation);
			return simul_config.speed_compensation;;
		
		case TACOM_INFO_RPM_COMPENSATION:
			printf("[SIMUL] tacom_get_info.. rpm compensation  : %s\n", simul_config.rpm_compensation);
			return simul_config.rpm_compensation;;
		
		case TACOM_INFO_CUMULATIVE_COMPENSATION:
			printf("[SIMUL] tacom_set_info.. cumulative compensation  : %s\n", simul_config.cumulative_compensation);
			return simul_config.cumulative_compensation;;
		
		case TACOM_INFO_MILEAGE_COMPENSATION:
			printf("[SIMUL] tacom_set_info.. mileage compensation  : %s\n", simul_config.mileage_compensation);
			return simul_config.mileage_compensation;;
		
		case TACOM_INFO_FUEL_COMPENSATION:
			printf("[SIMUL] tacom_set_info.. fuel compensation  : %s\n", simul_config.fuel_compensation);
			return simul_config.fuel_compensation;;
		
		default:
			_tm->tm_err.tm_errno = TACOM_INFO_TYPE_INVALIED;
			return "NOK";
	}

}

int
simul_get_info_hdr(TACOM *_tm, void *infohdr)
{
	tacom_std_hdr_t *hdr = infohdr;

	memcpy(hdr->vehicle_model, simul_config.vehicle_model, 20);
	memcpy(hdr->vehicle_id_num, simul_config.vehicle_id_num, 17);
	memcpy(hdr->vehicle_type, simul_config.vehicle_type, 2);
	memcpy(hdr->reg_num, simul_config.registration_num, 12);
	memcpy(hdr->business_license_number, simul_config.business_license_num, 10);
	memcpy(hdr->driver_code, simul_config.driver_code, 18);

	return 1;
}

int 
simul_set_info(TACOM *_tm, int type, char *infodata)
{
	switch (type) {
		case TACOM_INFO_VEHICLE_MODEL:
			memset(simul_config.vehicle_model, 0, sizeof(simul_config.vehicle_model));
			strcpy(simul_config.vehicle_model, infodata);
			printf("[SIMUL] tacom_set_info.. vehicle model : %s\n", infodata);
			break;
		case TACOM_INFO_VEHICLE_ID_NUM:
			memset(simul_config.vehicle_id_num, 0, sizeof(simul_config.vehicle_id_num));
			strcpy(simul_config.vehicle_id_num, infodata);
			printf("[SIMUL] tacom_set_info.. vehicle id num : %s\n", infodata);
			break;
		case TACOM_INFO_VEHICLE_TYPE:
			memset(simul_config.vehicle_type, 0, sizeof(simul_config.vehicle_type));
			strcpy(simul_config.vehicle_type, infodata);
			printf("[SIMUL] tacom_set_info.. vehicle type : %s\n", infodata);
			break;
		case TACOM_INFO_REGISTRATION_NUM:
			memset(simul_config.registration_num, 0, sizeof(simul_config.registration_num));
			strcpy(simul_config.registration_num, infodata);
			printf("[SIMUL] tacom_set_info.. registration num : %s\n", infodata);
			break;
		case TACOM_INFO_BUSINESS_LICENSE_NUM:
			memset(simul_config.business_license_num, 0, sizeof(simul_config.business_license_num));
			strcpy(simul_config.business_license_num, infodata);
			printf("[SIMUL] tacom_set_info.. business license num  : %s\n", infodata);
			break;
		case TACOM_INFO_DRIVER_CODE:
			memset(simul_config.driver_code, 0, sizeof(simul_config.driver_code));
			strcpy(simul_config.driver_code, infodata);
			printf("[SIMUL] tacom_set_info.. driver code  : %s\n", infodata);
			break;
		case TACOM_INFO_SPEED_COMPENSATION:
			memset(simul_config.speed_compensation, 0, sizeof(simul_config.speed_compensation));
			strcpy(simul_config.speed_compensation, infodata);
			printf("[SIMUL] tacom_set_info.. speed compensation  : %s\n", infodata);
			break;
		case TACOM_INFO_RPM_COMPENSATION:
			memset(simul_config.rpm_compensation, 0, sizeof(simul_config.rpm_compensation));
			strcpy(simul_config.rpm_compensation, infodata);
			printf("[SIMUL] tacom_set_info.. rpm compensation  : %s\n", infodata);
			break;
		case TACOM_INFO_CUMULATIVE_COMPENSATION:
			memset(simul_config.cumulative_compensation, 0, sizeof(simul_config.cumulative_compensation));
			strcpy(simul_config.cumulative_compensation, infodata);
			printf("[SIMUL] tacom_set_info.. cumulative compensation  : %s\n", infodata);
			break;
		case TACOM_INFO_MILEAGE_COMPENSATION:
			memset(simul_config.mileage_compensation, 0, sizeof(simul_config.mileage_compensation));
			strcpy(simul_config.mileage_compensation, infodata);
			printf("[SIMUL] tacom_set_info.. mileage compensation  : %s\n", infodata);
			break;
		case TACOM_INFO_FUEL_COMPENSATION:
			memset(simul_config.fuel_compensation, 0, sizeof(simul_config.fuel_compensation));
			strcpy(simul_config.fuel_compensation, infodata);
			printf("[SIMUL] tacom_set_info.. fuel compensation  : %s\n", infodata);
			break;
		default:
			_tm->tm_err.tm_errno = TACOM_INFO_TYPE_INVALIED;
			return -1;
	}
	return 1;

}
