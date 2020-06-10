<<<<<<< HEAD
#ifndef PACKET_H
#define PACKET_H


// return value...
typedef enum
{
	PACKET_RET_FAIL = -1,
	PACKET_RET_SUCCESS = 0,
	PACKET_RET_DONE = PACKET_RET_SUCCESS,
	PACKET_RET_MDT_PAKCET_FRAME_FULL,
	PACKET_RET_MDT_PAKCET_DATA_RANGE,
	PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE,
}PACKET_RET;

typedef enum
{
	PACKET_TRANSFER_STAT__COMPLETE = 0,
	PACKET_TRANSFER_STAT__SENDING
}PACKET_SEND_STAT;

// ---------------------------------------------
// for gtrack pkt send id
// ---------------------------------------------
#define MDS_PKT_1_EVENT 1

// ------------------------------------------
// 
// ------------------------------------------

typedef struct {
	unsigned char buff[512];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_1_HEAD;

typedef struct {
	unsigned char buff[256];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_1_CONTET;
// --------------------------------------------
//  
// --------------------------------------------

/*
typedef struct {
    unsigned char buff[512];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_2_HEAD;


typedef struct {
    unsigned char buff[256];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_2_CONTET;
*/


// -----------------------------------------
//  MDT packet Frame
// -----------------------------------------

#define e_MDS_PKT_1			0
//#define e_MDS_PKT_2			1
#define e_MDS_PKT_MAX		1

// 최대 저장할수있는 패킷의 수
#define MDS_PACKET_MGR_MAX_CNT					100

// 한개 패킷의 max body 갯수
#define MDS_PACKET_1_MAX_SAVE_CNT		300 // spare +5
//#define MDS_PACKET_2_MAX_SAVE_CNT 		300

#define MDS_PACKET_STAT_CLR					0x00
#define MDS_PACKET_STAT_USED				0x00
#define MDS_PACKET_STAT_PREPARE				0x02
#define MDS_PACKET_STAT_PREPARE_COMPLETE	0x04
#define MDS_PACKET_STAT_SEND_COMPLETE		0x08

#define ONCE_SEND_PACKET_MAX_CNT		99
// ---------------------------------------
// ---------------------------------------
typedef struct {
	MDS_PACKET_1_HEAD 	head;
	MDS_PACKET_1_CONTET body[MDS_PACKET_1_MAX_SAVE_CNT];
	unsigned int status;
	unsigned int body_cnt;
}__attribute__((packed))MDS_PACKET_1_MGR;

/*
typedef struct {
	MDS_PACKET_2_HEAD 	head;
	MDS_PACKET_2_CONTET body[MDS_PACKET_2_MAX_SAVE_CNT];
	unsigned int status;
	unsigned int body_cnt;	
}__attribute__((packed))MDS_PACKET_2_MGR;
*/


// ---------------------------------------
// packet manage api
// ---------------------------------------

int mds_packet_1_make_and_insert(char* input_buff, int input_size);
int mds_packet_1_make_done(char* input_buff, int input_size);
int mds_packet_1_get_front_pkt(unsigned short *size, unsigned char **pbuf);
int mds_packet_1_get_use_cnt();
int mds_packet_1_get_cur_body_size();
int mds_packet_1_clear_rear();
int mds_packet_1_get_rear_idx();
int flush_mds_packet_1(const int pkt_count);
int mds_packet_1_set_send_stat(int stat);
int mds_packet_1_get_send_stat();

// ---------------------------------------
// ---------------------------------------



// ---------------------------------------------
// packet index util..
// ---------------------------------------------
int __get_cur_packet_index_front(unsigned int packet);
int __get_cur_packet_index_rear(unsigned int packet);
int __get_cur_packet_use_cnt(unsigned int packet);
int __get_cur_packet_free_cnt(unsigned int packet);
int __set_increase_packet_front(unsigned int packet);
int __set_increase_packet_rear(unsigned int packet);
int __set_increase_use_cnt(int packet);
int __set_decrease_use_cnt(int packet);




#endif /* PACKET_H */
=======
#ifndef PACKET_H
#define PACKET_H


// return value...
typedef enum
{
	PACKET_RET_FAIL = -1,
	PACKET_RET_SUCCESS = 0,
	PACKET_RET_DONE = PACKET_RET_SUCCESS,
	PACKET_RET_MDT_PAKCET_FRAME_FULL,
	PACKET_RET_MDT_PAKCET_DATA_RANGE,
	PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE,
}PACKET_RET;

typedef enum
{
	PACKET_TRANSFER_STAT__COMPLETE = 0,
	PACKET_TRANSFER_STAT__SENDING
}PACKET_SEND_STAT;

// ---------------------------------------------
// for gtrack pkt send id
// ---------------------------------------------
#define MDS_PKT_1_EVENT 1

// ------------------------------------------
// 
// ------------------------------------------

typedef struct {
	unsigned char buff[512];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_1_HEAD;

typedef struct {
	unsigned char buff[256];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_1_CONTET;
// --------------------------------------------
//  
// --------------------------------------------

/*
typedef struct {
    unsigned char buff[512];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_2_HEAD;


typedef struct {
    unsigned char buff[256];
	unsigned int buff_size;
}__attribute__((packed))MDS_PACKET_2_CONTET;
*/


// -----------------------------------------
//  MDT packet Frame
// -----------------------------------------

#define e_MDS_PKT_1			0
//#define e_MDS_PKT_2			1
#define e_MDS_PKT_MAX		1

// 최대 저장할수있는 패킷의 수
#define MDS_PACKET_MGR_MAX_CNT					100

// 한개 패킷의 max body 갯수
#define MDS_PACKET_1_MAX_SAVE_CNT		300 // spare +5
//#define MDS_PACKET_2_MAX_SAVE_CNT 		300

#define MDS_PACKET_STAT_CLR					0x00
#define MDS_PACKET_STAT_USED				0x00
#define MDS_PACKET_STAT_PREPARE				0x02
#define MDS_PACKET_STAT_PREPARE_COMPLETE	0x04
#define MDS_PACKET_STAT_SEND_COMPLETE		0x08

#define ONCE_SEND_PACKET_MAX_CNT		99
// ---------------------------------------
// ---------------------------------------
typedef struct {
	MDS_PACKET_1_HEAD 	head;
	MDS_PACKET_1_CONTET body[MDS_PACKET_1_MAX_SAVE_CNT];
	unsigned int status;
	unsigned int body_cnt;
}__attribute__((packed))MDS_PACKET_1_MGR;

/*
typedef struct {
	MDS_PACKET_2_HEAD 	head;
	MDS_PACKET_2_CONTET body[MDS_PACKET_2_MAX_SAVE_CNT];
	unsigned int status;
	unsigned int body_cnt;	
}__attribute__((packed))MDS_PACKET_2_MGR;
*/


// ---------------------------------------
// packet manage api
// ---------------------------------------

int mds_packet_1_make_and_insert(char* input_buff, int input_size);
int mds_packet_1_make_done(char* input_buff, int input_size);
int mds_packet_1_get_front_pkt(unsigned short *size, unsigned char **pbuf);
int mds_packet_1_get_use_cnt();
int mds_packet_1_get_cur_body_size();
int mds_packet_1_clear_rear();
int mds_packet_1_get_rear_idx();
int flush_mds_packet_1(const int pkt_count);
int mds_packet_1_set_send_stat(int stat);
int mds_packet_1_get_send_stat();

// ---------------------------------------
// ---------------------------------------



// ---------------------------------------------
// packet index util..
// ---------------------------------------------
int __get_cur_packet_index_front(unsigned int packet);
int __get_cur_packet_index_rear(unsigned int packet);
int __get_cur_packet_use_cnt(unsigned int packet);
int __get_cur_packet_free_cnt(unsigned int packet);
int __set_increase_packet_front(unsigned int packet);
int __set_increase_packet_rear(unsigned int packet);
int __set_increase_use_cnt(int packet);
int __set_decrease_use_cnt(int packet);




#endif /* PACKET_H */
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
