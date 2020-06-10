#ifndef __HTTP_THERMAL_PKT_H__
#define __HTTP_THERMAL_PKT_H__

#define THERMAL_SENSOR_VAL__INVALID -5555

#define THERMAL_SENSOR_FILTER__MIN  -999
#define THERMAL_SENSOR_FILTER__MAX  999

#define MAX_HTTP_THERMAL_PKT_PARSE_FAIL 3

typedef struct thermaldata
{
    int thermal_val_1;
    int thermal_val_2;
}thermaldata_t;

void ds_get_thermal_val(thermaldata_t* sensor_val);

int make_http_thermal_pkt__send_themal_val(unsigned char **pbuf, unsigned short *packet_len, thermaldata_t * param);
int parse_http_thermal_pkt__send_themal_val(unsigned char * buff, int len_buff);

#endif // __CL_RFID_PKT_H__
