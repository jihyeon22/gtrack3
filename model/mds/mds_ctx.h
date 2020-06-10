#ifndef MDS_CTX_H
#define MDS_CTX_H

typedef struct RUN_CONTEXT {
    int power_status;
	int last_power_status;
    TRIPHOS_COMMON_PACKET_TIME keyon_time;
    TRIPHOS_COMMON_PACKET_TIME keyoff_time;
	int keyon_gather_data_interval_sec;
	int keyon_send_to_data_interval_sec;
	int keyoff_gather_data_interval_sec;
	int keyoff_send_to_data_interval_sec;
	char device_phone_num[PACKET_DEFINE_TEL_NO_LEN];
	int server_stat;
	unsigned int server_sleep_time;
	int network_stat;
}RUN_CONTEXT_T;

int get_ctx_network(void);
int set_ctx_network(int status);

int get_ctx_power(void);
int set_ctx_power(int status);
int get_ctx_power_is_changed(void);

int set_ctx_server_stat(int status);
int get_ctx_server_stat(void);

int set_ctx_server_sleep(int sec);
int get_ctx_server_sleep(void);


int decrease_ctx_server_sleep(void);
int set_pkt_ctx_keyoff_time(int year, int month, int day, int hour, int min, int sec);

//extern RUN_CONTEXT_T run_ctx;

int set_ctx_keyon_gather_data_interval(int sec);
int get_ctx_keyon_gather_data_interval(void);
int set_ctx_keyon_send_to_data_interval(int sec);
int get_ctx_keyon_send_to_data_interval(void);
int set_ctx_keyoff_gather_data_interval(int sec);
int get_ctx_keyoff_gather_data_interval(void);
int set_ctx_keyoff_send_to_data_interval(int sec);
int get_ctx_keyoff_send_to_data_interval(void);


#endif /* PACKET_H */
