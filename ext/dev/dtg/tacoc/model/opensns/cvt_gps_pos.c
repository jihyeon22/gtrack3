// _COMMON_CPP_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cvt_gps_pos.h"


void Geod2ECEF(double lat, double lon, double hei, double *px, double *py, double *pz, double a, double b)
{
	double	lat_r = lat * M_PI / 180.;
    double	lon_r = lon * M_PI / 180.;
	
    double	f = (a - b) / a;
    double	sqre = 2 * f - f * f;
	
    double	N = a / sqrt(1 - sqre * sin(lat_r) * sin(lat_r));
	
    *px = (N + hei) * cos(lat_r) * cos(lon_r);
    *py = (N + hei) * cos(lat_r) * sin(lon_r);
    *pz = (N * (1 - sqre) + hei) * sin(lat_r);
}

void ECEF2Geod(double x, double y, double z, double *plat, double *plon, double *phei, double a, double b)
{
	double	p = sqrt(x * x + y * y);
    double	theta = atan2((z * a) , (p * b));
    double	sqrep = (a * a - b * b) / (b * b);
	
    double	f = (a - b) / a;
    double	sqre = 2 * f - f * f;
	
    double	lat_r =
		atan2((z + sqrep * b * sin(theta) * sin(theta) * sin(theta)) ,
		(p - sqre * a * cos(theta) * cos(theta) * cos(theta)));
    double	lon_r = atan2(y, x);
	
    *plat = lat_r * 180. / M_PI;
    *plon = lon_r * 180. / M_PI;
    *phei = p / cos(lat_r) - a / sqrt(1 - sqre * sin(lat_r) * sin(lat_r));
}

void bessel2wgs(unsigned long bx1, unsigned long by1, double *pwx, double *pwy)
{
	double	X, Y, Z, H;
	double bx = bx1, by = by1;

	bx = bx / 36000.;
	by = by / 36000.;

	Geod2ECEF(by, bx, 0., &X, &Y, &Z, BESSEL_A, BESSEL_B);
	X += B2W_DELTAX;
	Y += B2W_DELTAY;
	Z += B2W_DELTAZ;
	ECEF2Geod(X, Y, Z, pwy, pwx, &H, WGS84_A, WGS84_B);
}

void deg2rad(double *ptx, double *pty)
{
    (*ptx) *= (M_PI/180.0);
    (*pty) *= (M_PI/180.0);
}

void wgs2bessel(double wx, double wy, unsigned long *pbx, unsigned long *pby)
{
    double	rn, rm, d_pi, d_lamda, d_h;
    double	h, bx, by;

    bx = wx;
    by = wy;
    h = 0.0;

    deg2rad(&wx, &wy);

    rn = WGS84_A / sqrt(1 - WGS84_EE*pow(sin(wy), 2.));
    rm = WGS84_A*(1-WGS84_EE) / pow(sqrt(1 - WGS84_EE*pow(sin(wy), 2.)), 3.);

    d_pi = (-W2B_DELTAX * sin(wy) * cos(wx) -
	    W2B_DELTAY * sin(wy) * sin(wx) +
	    W2B_DELTAZ * cos(wy) +
	    W2B_DELTAA * (rn * WGS84_EE * sin(wy) * cos(wy)) / WGS84_A +
	    W2B_DELTAF * (rm * WGS84_A / WGS84_B + rn * WGS84_B / WGS84_A) *
	    sin(wy) * cos(wy)) /
	    ((rm + h) * sin(M_PI/180. * 1/3600.));
    d_lamda = (-W2B_DELTAX * sin(wx) + W2B_DELTAY * cos(wx))
	      / ((rn + h) * cos(wy) * sin(M_PI/180. * 1/ 3600.));

    d_h = W2B_DELTAX * cos(wy) * cos(wx) +
	  W2B_DELTAY * cos(wy) * sin(wx) +
	  W2B_DELTAZ * sin(wy) - W2B_DELTAA * WGS84_A / rn +
	  W2B_DELTAF * WGS84_B / WGS84_A * rn * pow(sin(wy), 2.);

//printf("wgs2bessel, %f %f \n", d_lamda, d_pi);

    bx += d_lamda / 3600.0;
    by += d_pi / 3600.0;

//printf("wgs2bessel, %f %f\n", bx, by);

	bx *= 36000.;
	by *= 36000.;

//printf("wgs2bessel, %f %f\n", bx, by);

	*pbx = (unsigned long)(bx);// X ÁÂÇ¥ double => unsigned long *
	*pby = (unsigned long)(by);// Y ÁÂÇ¥ double => unsigned long *
}

/*
int main(){
  double wx = 127.101627;
  double wy = 37.3997830;
  unsigned long pbx;
  unsigned long pby;
  printf("start\n");

  printf("%f %f \n", wx, wy);

  wgs2bessel(wx, wy, &pbx, &pby);

  printf("%d %d \n", pbx, pby);

  printf("end\n");
  return 0;
}

*/

int ConvertStringTime2DwordTime(char *psztime_in, unsigned int *pdwTime)
{
	char sztmp[64];
	if(!psztime_in || !pdwTime || (strlen(psztime_in) < 14))
		return 0;
	
	//int TIME_U_GET_YEAR_MASK = 0x0000001F, TIME_U_GET_MONTH_MASK = 0x000001E0;
	//int TIME_U_GET_DAY_MASK = 0x00007E00, TIME_U_GET_HOUR_MASK = 0x000F8000;
	//int TIME_U_GET_MIN_MASK = 0x03F00000, TIME_U_GET_SEC_MASK = 0xFC000000;
	int TIME_U_GET_YEAR_BIT_LENGTH = 5, TIME_U_GET_MONTH_BIT_LENGTH = 4;
	int TIME_U_GET_DAY_BIT_LENGTH = 6, TIME_U_GET_HOUR_BIT_LENGTH = 5;
	int TIME_U_GET_MIN_BIT_LENGTH = 6;
	//int TIME_U_GET_SEC_BIT_LENGTH = 6;
	
	unsigned char byYear = 0, byMonth = 0, byDay = 0, byHour = 0, byMin = 0, bySec = 0;
	memset(sztmp, 0x00, sizeof(sztmp)); memcpy(sztmp, psztime_in + 2, 2); byYear = atoi(sztmp);
	memset(sztmp, 0x00, sizeof(sztmp)); memcpy(sztmp, psztime_in + 4, 2); byMonth = atoi(sztmp);
	memset(sztmp, 0x00, sizeof(sztmp)); memcpy(sztmp, psztime_in + 6, 2); byDay = atoi(sztmp);
	memset(sztmp, 0x00, sizeof(sztmp)); memcpy(sztmp, psztime_in + 8, 2); byHour = atoi(sztmp);
	memset(sztmp, 0x00, sizeof(sztmp)); memcpy(sztmp, psztime_in + 10, 2); byMin = atoi(sztmp);
	memset(sztmp, 0x00, sizeof(sztmp)); memcpy(sztmp, psztime_in + 12, 2); bySec = atoi(sztmp);

	if(byHour == 0)
		byHour = 24;

	if(byMonth > 12 || byDay > 31 || byHour > 24) 
		return 0;

	//printf("==> [%02d/%02d/%02d %02d:%02d:%02d]\n", byYear, byMonth, byDay, byHour, byMin, bySec);
	
	unsigned int dwTime = 0;
	dwTime = dwTime | byYear;
	dwTime = dwTime | (byMonth << TIME_U_GET_YEAR_BIT_LENGTH);
	dwTime = dwTime | (byDay << (TIME_U_GET_YEAR_BIT_LENGTH + TIME_U_GET_MONTH_BIT_LENGTH));
	dwTime = dwTime | (byHour << (TIME_U_GET_YEAR_BIT_LENGTH + TIME_U_GET_MONTH_BIT_LENGTH + TIME_U_GET_DAY_BIT_LENGTH));
	dwTime = dwTime | (byMin << (TIME_U_GET_YEAR_BIT_LENGTH + TIME_U_GET_MONTH_BIT_LENGTH + TIME_U_GET_DAY_BIT_LENGTH + TIME_U_GET_HOUR_BIT_LENGTH));
	dwTime = dwTime | (bySec << (TIME_U_GET_YEAR_BIT_LENGTH + TIME_U_GET_MONTH_BIT_LENGTH + TIME_U_GET_DAY_BIT_LENGTH + TIME_U_GET_HOUR_BIT_LENGTH + TIME_U_GET_MIN_BIT_LENGTH));
	*pdwTime = dwTime;

	return 1;
}
