#ifndef __DTG_DATA_PACKET_DEF_HEADER__
#define __DTG_DATA_PACKET_DEF_HEADER__

//network을 통해 한번에 보내를 Max Data Size
//이를 초과시 ONE_PACKET_SIZE 크기 만큼 나누어서 서버로 전송한다.
#define ONE_PACKET_SIZE		1000

//서버로 부터 Packet을 wait하는 Max Time이다.
//1의 값은 usleep(100)에 해당한다.
#define RECEIVED_MAX_TIME_OUT		30 //usleep(100) * RECEIVED_MAX_TIME_OUT is max waiting time


//SEND_MAX_RETRY_COUNT은 client에서 sent 자체 error 발생의 경우
#define SEND_MAX_RETRY_COUNT			3

//MAX_PACKET_MISS_ERROR_COUNT은 통신은 되었으나
//Packet loss 및 broken으로 인해 retry하는 경우
#define MAX_PACKET_MISS_ERROR_COUNT		5 



#define PACKET_PAY_LOAD_SIZE	1024
#define PHONE_NUMBER_LEN		12
#define UPINFO_IP				32
#define UPINFO_ID				32
#define UPINFO_PW				32
#define UPINFO_FN				256


#pragma pack(push, 1)
//서버 전송시 최초(First Packet) 전송은 관련 Header 정보를 더 추가해서 보낸다.
typedef struct {
	unsigned short crc16;
	int msg_type;
	unsigned short device_type; //조영, 신흥, UCAR 등등
	int sent_packet_size;
	unsigned short packet_num;
	int origin_length;
	int compress_length;
	unsigned char package_version_major;
	unsigned char package_version_minor;
	char phone_num[PHONE_NUMBER_LEN];
}__attribute__((packed))PACKET_DATA_HDR;
typedef struct {
	PACKET_DATA_HDR hdr;
	unsigned char p_data[PACKET_PAY_LOAD_SIZE];
}__attribute__((packed))PACKET_DATA;

//최초 (First packet)을 제외한 모든 Packet은 Header Info를 축소해서 보낸다.
typedef struct {
	unsigned short crc16;
	int msg_type;
	int sent_packet_size;
	unsigned short packet_num;
	unsigned char dummy[4];
}__attribute__((packed))PACKET_NEXT_HDR;
typedef struct {
	PACKET_NEXT_HDR hdr;
	unsigned char p_data[PACKET_PAY_LOAD_SIZE];
}__attribute__((packed))PACKET_NEXT;

typedef struct{
	char update_server_ip[UPINFO_IP];
	int update_server_port;
	char id[UPINFO_ID];
	char passwrd[UPINFO_PW];
	char file_path[UPINFO_FN];
}__attribute__((packed))UPDATE_INFO_RESPONSE;

#pragma pack(pop)

//서버 전송시 Message Type Field에 넣을 수 있는 값임.
//Or 연산을 통해 중복 type으로 보낼 수 있음.
#define MSG_TYPE_DTG_DATA					0x00000001
#define MSG_TYPE_DIAG_DATA					0x00000002
#define MSG_TYPE_UPDATE_INFO_REQUEST		0x00000004
//#define MSG_TYPE_RESERVE					0x00000008
//#define MSG_TYPE_RESERVE					0x00000010
#define MSG_TYPE_REGISTRATION				0x00000020
#define MSG_TYPE_DE_REGISTRATION			0x00000040
#define MSG_TYPE_DTG_BREAKDOWN				0x00000080
#define MSG_FIRST_PACKET_DATA				0x00000100
#define MSG_CONTINUE_DATA					0x00000200

#define MSG_FINISH_DATA						0x80000000




//프로그램 내부에서 구현한 Network 함수에서 Return하는 Type들임.
#define NET_SUCCESS_OK						(0)	  //정상
#define NET_SUCCESS_UPDATE_INFO				(-1)  //정상(예외적으로 음수이지만 정상 Return이다.
#define NET_SOCKET_OPEN_FAIL				(-2)  //socket open fail
#define NET_SERVER_CONNECT_ERROR			(-3)  //server connection fail
#define NET_SEND_PACKET_ERROR				(-4)  //packet send fail into server
#define NET_RECV_PACKET_ERROR				(-5)  //packet receive fail from server
#define NET_RECV_PACKET_TIME_OUT			(-6)  //packet receive time out
#define NET_SEND_OK_BUT_RESP_ERROR			(-7)  //서버로 부터 response code가 error(RETURN_OK 이외의 값)를 수신
#define NET_PACKAGE_UPDATE_NEED				(-8)  //서버로 부터 업데이트 필요 command 받음.
//프로그램 내부에서 구현한 Network 함수에서 Return하는 Type들임. --


//서버로부터 수신되는 Response 값
#define RETURN_OK							1
#define RETURN_RECV_DATA_FAIL				2  //서버에서 recv 하다가 fail 발생시
#define RETURN_UNCOMPRESS_FAIL				3  //압축된 Data를 풀었을 때 Error 발생 시
#define RETURN_DATA_LENGTH_ERROR			4  //서버에서 받은 size와 client header에서 보내준 length가 다른 경우
#define RETURN_UNKNOWN_VERSION				5  //서버에서 알 수 없는 Client Version을 보냈을 시
#define RETURN_ALREADY_RECEIVED				6  //순차적인 packet number가 다른 경우 packet이 왔을 시
#define RETURN_DATA_CRC_ERROR				7  //packet의 crc가 다른 경우
#define RETURN_MSG_TYPE_MISMATCH_ERROR		8  //packet의 crc가 다른 경우
#define RETURN_DATA_SAVE_ERROR				9  //DTG 데이터 저장 Error

//moon ++
#define RETURN_REG_FAIL                    10
#define RETURN_DEREG_FAIL				   11
#define RETURN_DTG_INFO_DB_UPDATE_FAIL	   12
//moon --
#define RETURN_UPDATE_MUST_BE				20
#define RETURN_UPDATE_INFO_ERROR			21
//서버로부터 수신되는 Response 값 --

#endif
