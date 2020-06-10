#ifndef __CL_ADAS_MGR_H__
#define __CL_ADAS_MGR_H__

int lila_adas_mgr__init();


typedef enum adas_evt_code adas_evt_code_t;
enum adas_evt_code{
	eADAS_CODE__FCW,
    eADAS_CODE__PCW,
    eADAS_CODE__HMW,
    eADAS_CODE__LDW_R,
    eADAS_CODE__LDW_L,
};

typedef struct {
    unsigned int time_sec;             // Reserved 	Binary 	4
    adas_evt_code_t evt_code;
    int ext_data;
}__attribute__((packed))LILA_ADAS__DATA_T; // 256 byte


#endif // __CL_ADAS_MGR_H__
