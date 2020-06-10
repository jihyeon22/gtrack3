#pragma once

#define KT_FOTA_PROTOCOL_VER	"1.0"

typedef enum KT_Fota_Svc KT_Fota_Svc_t;
enum KT_Fota_Svc
{
	eKT_FOTA_REQ,
	eKT_DEVICE_QTY_NOTI,
	eKT_DEVICE_ON_NOTI,
	eKT_DEVICE_OFF_NOTI,
	eKT_MODEM_QTY_NOTI,
	eKT_MAX_FOTA_SVC,
};

typedef enum KT_Fota_Req_Mode KT_Fota_Req_Mode_t;
enum KT_Fota_Req_Mode
{
	ePOLLING_MODE,
	ePUSH_MODE,
};

typedef enum KT_Fota_Qty_Mode KT_Fota_Qty_Mode_t;
enum KT_Fota_Qty_Mode
{
	eMODEM_QTY_MODE,
	eDEVICE_MODEM_QTY_MODE,
};

typedef enum KT_Fota_DL_Mode KT_Fota_DL_Mode_t;
enum KT_Fota_DL_Mode
{
	eHTTP,
	eFTP,
};

typedef enum KT_Fota_Retun_Code KT_Fota_Retun_Code_t;
enum KT_Fota_Retun_Code
{
	eRC_FAIL_VER              = 101,  //fail,    protocol version error
	eRC_OK_NEED_UPDATE        = 200,  //success, server have more last version binary than current device. so need binary update.
	eRC_OK_LAST_VERSION       = 204,  //success, binary version of device is latest version. so, it need NOT binary update.
	eRC_OK_DEV_PWR_ON         = 210,  //success, device power on notification 
	eRC_FAIL_ACCOUNT          = 401,  //fail,    account certification error.
	eRC_FAIL_FILE_NOT_FOUND   = 404,  //fail,    File Not Found. 
	eRC_FAIL_FOTA_DISAPPROVE  = 405,  //fail,    FOTA  disapprove
	eRC_FAIL_PRAMETER         = 406,  //fail,    parameter error
	eRC_FAIL_USER_AUTH        = 407,  //fail,    KT user authentication
	eRC_FAIL_DEVICE_NOT_FOUNT = 408,  //fail,    Device Not Found
	eRC_OK_DEV_ON_NOTI        = 410,  //success, Device On Noti success
	eRC_FAIL_SERVER_BUSY      = 503,  //fail,    FOTA SERVER BUSY
	
	
	
	
};

int send_device_on_packet(int time_out);
int send_fota_req_packet(KT_Fota_Req_Mode_t req_type, KT_Fota_DL_Mode_t dl_mode, int time_out);
int send_device_off_packet(int time_out);
int send_qty_packet(KT_Fota_Req_Mode_t req_type, KT_Fota_Qty_Mode_t qty_mode, int time_out);

