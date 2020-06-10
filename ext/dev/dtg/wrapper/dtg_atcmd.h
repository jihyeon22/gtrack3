<<<<<<< HEAD
#ifndef __DTG_ATCMD_H__
#define __DTG_ATCMD_H__


extern int atcmd_errno;

int atcmd_errno;

char* atcmd_get_phonenum();
char* atcmd_get_imei();
int atcmd_get_rssi();
int atcmd_send_sms(const char *message, const char *sender, const char *receiver);

#endif // __DTG_ATCMD_H__
=======
#ifndef __DTG_ATCMD_H__
#define __DTG_ATCMD_H__


extern int atcmd_errno;

int atcmd_errno;

char* atcmd_get_phonenum();
char* atcmd_get_imei();
int atcmd_get_rssi();
int atcmd_send_sms(const char *message, const char *sender, const char *receiver);

#endif // __DTG_ATCMD_H__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
