#ifndef _TACOM_PROTOCOL_H_
#define _TACOM_PROTOCOL_H_

/* server protocol */
#include <tacom/tacom_std_protocol.h>

/* DTG protocol */
#if defined (DEVICE_MODEL_SINHUNG)
	#include <tacom_sh_protocol.h>
#elif defined (DEVICE_MODEL_UCAR)
	#include <tacom_ucar_protocol.h>
#elif defined (DEVICE_MODEL_LOOP)
	#include <tacom_loop_protocol.h>
#elif defined (DEVICE_MODEL_CHOYOUNG) || defined (DEVICE_MODEL_KDT)
	#include <tacom_choyoung_protocol.h>
#elif defined (DEVICE_MODEL_IREAL)
	#include <tacom_ireal_protocol.h>
#elif defined (DEVICE_MODEL_INNOCAR) || defined (DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
	#include <tacom_innocar_protocol.h>
#elif defined (DEVICE_MODEL_DAESIN)
	#include <tacom_daesin_protocol.h>
#elif defined (DEVICE_MODEL_CJ)
	#include <tacom_cj_protocol.h>
#elif defined (DEVICE_MODEL_LOOP2)
	#include <tacom_new_loop.h>
#else
	#error "DTG MODEL NOT DEFINE ERROR!!!!!"
#endif

#endif 
