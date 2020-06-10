#ifndef _COMMON_H_
#define _COMMON_H_

#include <math.h>

//#define M_PI ((double)3.14159265358979323846)

#define BESSEL_A	6377397.155	
#define BESSEL_RF	299.1528128
#define BESSEL_B	(BESSEL_A - BESSEL_A / BESSEL_RF)
#define BESSEL_EE	(0.006674372231315)	// (a*a - b*b)/(a*a)

/** WGS84 */
#define WGS84_A		6378137.0
#define WGS84_F		(1.0 / 298.257223563)
#define WGS84_RF	298.257223563
#define WGS84_B		(WGS84_A - WGS84_A / WGS84_RF)
#define WGS84_EE	(2.0 * WGS84_F - WGS84_F * WGS84_F)

/** B2W */
#define B2W_DELTAX	-147
#define B2W_DELTAY	506
#define B2W_DELTAZ	687

#define W2B_DELTAX	128
#define W2B_DELTAY	-481
#define W2B_DELTAZ	-664
#define W2B_DELTAA	-739.845
#define W2B_DELTAF	-0.000010037483

//extern void Geod2ECEF(double lat = 0x00, double lon = 0x00, double hei = 0x00, double *px = 0x00, double *py = 0x00, double *pz = 0x00, double a = 0x00, double b = 0x00);
//extern void ECEF2Geod(double x = 0x00, double y = 0x00, double z = 0x00, double *plat = 0x00, double *plon = 0x00, double *phei = 0x00, double a = 0x00, double b = 0x00);
//extern void bessel2wgs(unsigned long bx1 = 0x00, unsigned long by1 = 0x00, double *pwx = 0x00, double *pwy = 0x00);
//extern void deg2rad(double *ptx = 0x00, double *pty = 0x00);
//extern void wgs2bessel(double wx = 0x00, double wy = 0x00, unsigned long *pbx = 0x00, unsigned long *pby = 0x00);


extern void Geod2ECEF(double lat, double lon, double hei, double *px, double *py, double *pz, double a, double b);
extern void ECEF2Geod(double x, double y, double z, double *plat, double *plon, double *phei, double a, double b);
extern void bessel2wgs(unsigned long bx1, unsigned long by1, double *pwx, double *pwy);
extern void deg2rad(double *ptx, double *pty);
extern void wgs2bessel(double wx, double wy, unsigned long *pbx, unsigned long *pby);
int ConvertStringTime2DwordTime(char *psztime_in, unsigned int *pdwTime);
#endif // _COMMON_H_
