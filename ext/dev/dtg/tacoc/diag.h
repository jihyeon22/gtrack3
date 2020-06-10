#ifndef _DIAG_H_
#define _DIAG_H_

/* System */
#define		DMESG				0x0001
#define		PS					0x0002
#define		MEMINFO				0x0004
#define		FILESYSTEM			0x0008
#define		UPTIME				0x0010
#define		LINUX_VERSION		0x0020

/* Modem */
#define		DBGSCRN				0x0040

/* Application */
#define		PACKAGE_INFO		0x0080
#define		CONFIG_INFO			0x0100

#define		DIAG_ALL			0xFFFF

int create_diag_info(int flag);
#endif
