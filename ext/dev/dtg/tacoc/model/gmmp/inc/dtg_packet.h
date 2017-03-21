#ifndef __DTG_DATA_PACKET_DEF_HEADER__
#define __DTG_DATA_PACKET_DEF_HEADER__

//network�� ���� �ѹ��� ������ Max Data Size
//�̸� �ʰ��� ONE_PACKET_SIZE ũ�� ��ŭ ����� ������ �����Ѵ�.
#define ONE_PACKET_SIZE		1000

//������ ���� Packet�� wait�ϴ� Max Time�̴�.
//1�� ���� usleep(100)�� �ش��Ѵ�.
#define RECEIVED_MAX_TIME_OUT		30 //usleep(100) * RECEIVED_MAX_TIME_OUT is max waiting time


//SEND_MAX_RETRY_COUNT�� client���� sent ��ü error �߻��� ���
#define SEND_MAX_RETRY_COUNT			3

//MAX_PACKET_MISS_ERROR_COUNT�� ����� �Ǿ�����
//Packet loss �� broken���� ���� retry�ϴ� ���
#define MAX_PACKET_MISS_ERROR_COUNT		5 



#define PACKET_PAY_LOAD_SIZE	1024
#define PHONE_NUMBER_LEN		12
#define UPINFO_IP				32
#define UPINFO_ID				32
#define UPINFO_PW				32
#define UPINFO_FN				256


#pragma pack(push, 1)
//���� ���۽� ����(First Packet) ������ ���� Header ������ �� �߰��ؼ� ������.
typedef struct {
	unsigned short crc16;
	int msg_type;
	unsigned short device_type; //����, ����, UCAR ���
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

//���� (First packet)�� ������ ��� Packet�� Header Info�� ����ؼ� ������.
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

//���� ���۽� Message Type Field�� ���� �� �ִ� ����.
//Or ������ ���� �ߺ� type���� ���� �� ����.
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




//���α׷� ���ο��� ������ Network �Լ����� Return�ϴ� Type����.
#define NET_SUCCESS_OK						(0)	  //����
#define NET_SUCCESS_UPDATE_INFO				(-1)  //����(���������� ���������� ���� Return�̴�.
#define NET_SOCKET_OPEN_FAIL				(-2)  //socket open fail
#define NET_SERVER_CONNECT_ERROR			(-3)  //server connection fail
#define NET_SEND_PACKET_ERROR				(-4)  //packet send fail into server
#define NET_RECV_PACKET_ERROR				(-5)  //packet receive fail from server
#define NET_RECV_PACKET_TIME_OUT			(-6)  //packet receive time out
#define NET_SEND_OK_BUT_RESP_ERROR			(-7)  //������ ���� response code�� error(RETURN_OK �̿��� ��)�� ����
#define NET_PACKAGE_UPDATE_NEED				(-8)  //������ ���� ������Ʈ �ʿ� command ����.
//���α׷� ���ο��� ������ Network �Լ����� Return�ϴ� Type����. --


//�����κ��� ���ŵǴ� Response ��
#define RETURN_OK							1
#define RETURN_RECV_DATA_FAIL				2  //�������� recv �ϴٰ� fail �߻���
#define RETURN_UNCOMPRESS_FAIL				3  //����� Data�� Ǯ���� �� Error �߻� ��
#define RETURN_DATA_LENGTH_ERROR			4  //�������� ���� size�� client header���� ������ length�� �ٸ� ���
#define RETURN_UNKNOWN_VERSION				5  //�������� �� �� ���� Client Version�� ������ ��
#define RETURN_ALREADY_RECEIVED				6  //�������� packet number�� �ٸ� ��� packet�� ���� ��
#define RETURN_DATA_CRC_ERROR				7  //packet�� crc�� �ٸ� ���
#define RETURN_MSG_TYPE_MISMATCH_ERROR		8  //packet�� crc�� �ٸ� ���
#define RETURN_DATA_SAVE_ERROR				9  //DTG ������ ���� Error

//moon ++
#define RETURN_REG_FAIL                    10
#define RETURN_DEREG_FAIL				   11
#define RETURN_DTG_INFO_DB_UPDATE_FAIL	   12
//moon --
#define RETURN_UPDATE_MUST_BE				20
#define RETURN_UPDATE_INFO_ERROR			21
//�����κ��� ���ŵǴ� Response �� --

#endif
