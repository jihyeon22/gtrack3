
#include <time.h>

#include <base/devel.h>
#include <base/sender.h>
#include <at/at_util.h>

#include <logd_rpc.h>
#include "netcom.h"


#include "ext/rfid/cl_rfid_pkt.h"
#include "ext/rfid/cl_rfid_tools.h"
#include "ext/rfid/cl_rfid_tools.h"

#include "ext/rfid/cust2_rfid/cust2_rfid_tools.h"
#include "ext/rfid/cust2_rfid/cust2_rfid_cmd.h"

#include "ext/rfid/cust2_rfid/cust2_rfid_senario.h"

#define LOG_TARGET eSVC_MODEL

void cust2_rfid__init()
{
    init_cust2_rfid();
    
}

void cust2_rfid___main_senario()
{

    if ( cust2_rfid__get_senario() == CUST2_RFID_SENARIO__NEED_INIT )
    {
        cust2_rfid_cmd__device_init();
        cust2_rfid__set_senario(CUST2_RFID_SENARIO__DOING_INIT);
    }

}

static int _senario_flag = CUST2_RFID_SENARIO__NEED_INIT;
void cust2_rfid__set_senario(int flag)
{
    printf("cust2 set sernaio [%d] => [%d]\r\n",_senario_flag, flag );
    _senario_flag = flag;
    return _senario_flag;
}

int cust2_rfid__get_senario()
{
    printf("cust2 get sernaio [%d] \r\n",_senario_flag );
    return _senario_flag;
}

int cust2_rfid__rfid_read_proc(char* buff)
{
 	time_t t;
    struct tm *lt;
    char   time_str[26];

    char boarding_list[1024] = {0,};
    
    memset(&boarding_list, 0x00, sizeof(boarding_list));
    
    strcpy(boarding_list, buff);

    sender_add_data_to_buffer(PACKET_TYPE_HTTP_SET_BOARDING_LIST, &boarding_list, ePIPE_2);

    cust2_rfid_cmd__send_code(GET_PASSENGER_DATA__SUCCESS);
}