#pragma once

#include "lila_packet.h"
#include "dtg_gtrack_tool.h"

void lila_dtg_debug__print_frame_header(LILA_PKT_FRAME__HEADER_T* p_frame_header);
void lila_dtg_debug__print_dtg_info(LILA_PKT_DATA__DTG_INFO_T* p_dtg_info);
void lila_dtg_debug__print_frame_tail(LILA_PKT_FRAME__TAIL_T* p_frame_tail);
void lila_dtg_debug__print_dtg_data(LILA_PKT_DATA__DATA_T* p_dtg_data);

void lila_dtg_debug__print_tacom_std_data(tacom_std_data_t* p_current_std_data);
void lila_dtg_debug__print_tacom_std_hdr(tacom_std_hdr_t* p_current_std_hdr);
// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

