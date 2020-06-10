
#include <stdio.h>
#include <unistd.h>

#include <netcom.h>
#include <base/devel.h>
#include <base/sender.h>

#include "logd/logd_rpc.h"

#include "ext/rfid/cl_rfid_tools.h"

#define LOG_TARGET eSVC_MODEL


void rfid_main_senario_init()
{
#ifdef USE_KJTEC_RFID
    kjtec_rfid___init();
#endif
#ifdef USE_SUP_RFID
    sup_rfid__init();
#endif
#ifdef USE_CUST2_RFID
    cust2_rfid__init();
#endif

#ifdef USE_CUST1_RFID
    cust1_rfid__init();
#endif

}


void rfid_main_senario()
{
#ifdef USE_KJTEC_RFID
    kjtec_rfid___main_senario();
#endif
#ifdef USE_SUP_RFID
    // empty
#endif
#ifdef USE_CUST2_RFID
    cust2_rfid___main_senario();
#endif

#ifdef USE_CUST1_RFID
    // empty
#endif

}