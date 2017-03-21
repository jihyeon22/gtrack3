#ifndef _TACOM_INTERNAL_H_
#define _TACOM_INTERNAL_H_

#include <tacom_perr.h>
#include <pthread.h>
#include <tacom/tacom_std_protocol.h>
#include <tacom/tacom_inc.h>

#define DTG_TTY_DEV_NAME	"/dev/ttyHSL1"
//#define DTG_TTY_DEV_NAME	"/dev/ttyEXT"

enum tacom_stat {
	TACOM_IDLE = 0,
	TACOM_BUSY_NORMAL,
	TACOM_BUSY_BURST,
	TACOM_BREAKDOWN,
	TACOM_ERR,

	TACOM_STATUS_NOT_IDLE,
	TACOM_STATUS_NO_REQS,

	TACOM_UART_NO_REQS,
	TACOM_UART_READ_ERR,
	TACOM_UART_WRITE_ERR,

	TACOM_INFO_TYPE_INVALIED,
};

#define RL_COMMAND_ALERT	0
#define RS_COMMAND_ALERT	1
#define INITIATIVE_FAIL		2
#define CRC_ERROR_ALERT		3

struct dtg_stat {
	int status;
	int records;
	char vrn[12];
};
typedef struct dtg_stat dtg_status_t;

struct tm_stream {
	size_t size ;
	char *stream;
};

struct tacom_setup {
	const char *tacom_dev;
	const char *cmd_rl;
	const char *cmd_rs;
	const char *cmd_rq;
	const char *cmd_rr;
	const char *cmd_rc;
	const char *cmd_ack;
	const int data_rl;
	const int data_rs;
	const int data_rq;
	const char start_mark;
	const char end_mark;
	const int head_length;
	const char head_delim;
	const int head_delim_index;
	const int head_delim_length;
	const int record_length;
	const int max_records_size;
	const int max_records_per_once;
	unsigned long conf_flag;	
};

struct tacom {
	enum tacom_stat status;

	pthread_mutex_t sync_mutex;
	pthread_cond_t  sync_cond;

	struct tm_stream tm_strm;

	const struct tm_ops *tm_ops;
	struct tacom_setup *tm_setup;

	struct tm_err {
		int tm_errno;
	} tm_err;

	void *tm_op1;
	void *tm_op2;
};
typedef struct tacom TACOM;

struct tm_ops {
	int (*tm_init_process)();
	int (*tm_recv_data)(char *data, int len, char eoc);
	int (*tm_send_cmd)(const char *cmd, struct tacom_setup *tm_setup, char *caller_detector);
	int (*tm_read_summary)();
	int (*tm_ack_summary)();
	int (*tm_read_current)();
	int (*tm_current_record_parsing)(char *shbuf, int len, char *strm);
	int (*tm_unreaded_records_num)();
	int (*tm_read_records)( int r_num);
	int (*tm_record_parsing)(char *shbuf, int len, char *strm, int read_num);
	int (*tm_ack_records)(int r_num);
};


/* Configuration flag(conf_flag) Bits */

/* 12~15 of config flags bits(conf_flag) are state information */
#define RESERVED_BIT15			15
#define RESERVED_BIT14			14
#define RESERVED_BIT13			13
#define RESERVED_BIT12			12

#define HNRT_FORMAT_BIT			11
#define DSIC_FORMAT_BIT			10
#define LBC_FORMAT_BIT			9
#define XML_FORMAT_BIT			8

#define READ_CURRENT_BIT		6
#define SUMMARY_ENABLE_BIT		5
#define STANDARD_FLOW_BIT		4

#define TACOM_CRC16_BIT			2
#define TACOM_CRC_ENDIAN1_BIT	1
#define TACOM_CRC_ENDIAN0_BIT	0

struct tacom_list {
	const char *name;
	const struct tm_ops *ops;
	struct tacom_setup *setup;
};

extern int dtg_uart_fd;
extern int tacom_data_build_period;
extern int tacom_data_type;

TACOM *tacom_init ();
int tacom_control (int type, char *data);

TACOM* tacom_get_cur_context();
char *tacom_get_stream();

int tacom_get_errno();
int tacom_get_status ();
void tacom_set_status (enum tacom_stat stat);

void load_dtg_status();
void store_dtg_status();




int send_cmd(char *cmd, struct tacom_setup *tm_setup, char *caller_detector);
int recv_data(char *data, int len, char eoc, int b_size);
int unreaded_records_num();
int read_current();
int read_records (int r_num);
int ack_records (int r_num);
int custom_command(char *command, char *result_str, int result_str_len);

int crc_check(unsigned char * buf, int r_size, struct tacom_setup *tm_setup);

#include <tacom/tacom_protocol.h>

#endif // _TACOM_INTERNAL_H_
