#ifndef __MODEL_CIPRMC_H__
#define __MODEL_CIPRMC_H__
INT32 ciprmc_convert_wgs84Y(const float degree);
INT32 ciprmc_convert_wgs84X(const float degree);
BYTE ciprmc_convert_angle(const float azimuth);
WORD ciprmc_convert_speed(const float speed);
UINT32 ciprmc_convert_distance(const float mileage);
#endif

