<<<<<<< HEAD
#ifndef __GMMP_API_DEFINE_HEADER__
#define __GMMP_API_DEFINE_HEADER__

int GMMP_Module_Init();
int GMMP_Profile_Request();

#ifndef TACOC_STANDALONE
int get_phone_number(char* pn, int len);
int get_imei(char* imei, int len);
void mdmc_get_ip(char *ip);
int app_download(char *addr, int port, char *id,char *pass,char *file);
int app_update();
void setup_cip_header(GMMP_CIP_DATA_HEADER *packet,int type);
int send_dtg_etc_packet_to_gmmp(int type);
#endif

#endif
=======
#ifndef __GMMP_API_DEFINE_HEADER__
#define __GMMP_API_DEFINE_HEADER__

int GMMP_Module_Init();
int GMMP_Profile_Request();

#ifndef TACOC_STANDALONE
int get_phone_number(char* pn, int len);
int get_imei(char* imei, int len);
void mdmc_get_ip(char *ip);
int app_download(char *addr, int port, char *id,char *pass,char *file);
int app_update();
void setup_cip_header(GMMP_CIP_DATA_HEADER *packet,int type);
int send_dtg_etc_packet_to_gmmp(int type);
#endif

#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
