//For DRAGON Duali RFID reader.

#ifndef __BOARD_RFIDTOOL_H__
#define __BOARD_RFIDTOOL_H__

#define DEV_RFID	    "/dev/ttyMSM"
#define BAUDRATE		115200

typedef struct rfidHead rfidHead_t;
struct rfidHead
{
	unsigned char stx;
	unsigned char len_h;
	unsigned char len_l;
}__attribute__((packed));

typedef struct rfidTail rfidTail_t;
struct rfidTail
{
	unsigned char csum;
}__attribute__((packed));

typedef struct rfidData rfidData_t;
struct rfidData{
	unsigned char uid[10];
	int len_uid;
	int boarding;
	unsigned int ktime;
};

#define RFID_RF_ON_CMD 0x10
#define RFID_RF_OFF_CMD 0x11
#define RFID_FIND_CARD_CMD 0x4C
#define RFID_BUZZER_LED_CMD 0x13
#define RFID_LED_CMD 0xFF

#define RFID_OK_RESP 0x00
#define RFID_NO_TAG_ERROR 0x02

#define STX 0x02

#define MAX_RFID_ARRAY	1000
#define RFID_SKIP_SECS	30
#define RFID_LOG_PATH	"/var/log/rfid.log"

enum
{
	RFID_NOT_USED = 0,
	RFID_GET_ON,
	RFID_GET_OFF
};

int rfid_init(void);
int rfid_deinit(void);
int rfid_find_card(unsigned char *uid);
int rfid_beep(int octave, int frequency, int off_secs);
int rfid_clear_uart(void);
int rfid_check_passenser(unsigned char *uid, int len_uid);
void rfid_dump_var_log(void);

#endif
