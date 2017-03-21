#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>


#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <memory.h>
#include <malloc.h>

#include <signal.h>
#include <time.h>

#include <rpc/rpc.h>

#include "gmmp_net.h"
#include "gmmp_api.h"
#include "gmmp_manager.h"
#include "gmmp_utill.h"
#include "dtg_packet.h"

#include <pthread.h>

#ifndef TACOC_STANDALONE
extern CLIENT *clnt_taco;
#endif

int init_delivery_timer(void);
int set_delivery_timer(void);
static void delivery_handler(int sig, siginfo_t *si, void *uc); 

#ifndef TACOC_STANDALONE
#else
#include <termios.h>
extern void *g_simuldata;
extern unsigned int g_len_simuldata;

int g_modem_uart_fd = 0;
pthread_t tid_sms_thread;

typedef struct
{
	char time[15];
	char phone_number[12];
	char type;
	char data[80];
}SMSDATA;

int init_uart(char* dev, int baud , int *fd)
{
	struct termios newtio;

	*fd = open( dev, O_RDWR | O_NOCTTY | O_NONBLOCK );
	if ( *fd < 0 ) {
		printf("%s> uart dev '%s' open fail [%d]\n", __func__, dev, *fd);
		return -1;
	}

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts

	switch( baud )
	{
		case 115200 : newtio.c_cflag |= B115200; break;
		case 57600 : newtio.c_cflag |= B57600; break;
		case 38400 : newtio.c_cflag |= B38400; break;
		case 19200 : newtio.c_cflag |= B19200; break;
		case 9600 : newtio.c_cflag |= B9600; break;
		case 4800 : newtio.c_cflag |= B4800; break;
		case 2400 : newtio.c_cflag |= B2400; break;
		default : newtio.c_cflag |= B115200; break;
	}

	newtio.c_lflag = 0;
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1�� ����
	//newtio.c_cc[VMIN] = vmin; // �ּ� n ���� ���� ������ ����
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush ( *fd, TCIFLUSH );
	tcsetattr( *fd, TCSANOW, &newtio );
}

int parse_sms(char *string,char *data)
{
	char token[]=":";
	char token_1[]=",";
	SMSDATA smsdata;
	char *psms;
	memset(&smsdata,0,sizeof(SMSDATA));
	psms=strtok(string,token);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strncpy(smsdata.time,psms,14);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strncpy(smsdata.phone_number,psms,11);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	smsdata.type=*(psms);
	psms=strtok(0,"\r");
	if(psms==0) return -1;
	strcpy(smsdata.data,psms);
	memcpy(data,smsdata.data,80);
	//printf("%s %s %c %s\n",smsdata.time,smsdata.phone_number,smsdata.type,smsdata.data);
	return 0;
}


void sms_thread(void)
{
	char buf[1024];
	char data[512];
	int readcnt;
	while(1){
		memset(buf,0,1024);
		readcnt = read(g_modem_uart_fd, buf, sizeof(buf));
		if(readcnt!=0){
			if(strstr(buf,"*SKT*NEWMT")!=0){
				printf("[%s]\n",buf);
				parse_sms(buf,data);
				printf("[%s]\n",data);
				GMMP_Control_Process(data,strlen(data),SMS);
			}
		}
		sleep(1);
	}
}
#endif

#ifndef TACOC_STANDALONE
int main_process()
#else
int main(int argc, char *argv[])
#endif
{
	int ret;

	gmmp_manager_init();
	load_ini_file();

#ifndef TACOC_STANDALONE
	//��ȭ��ȣ�� �������� ���ϸ� Critical�� �����̹Ƿ� ���� �ݺ��Ѵ�.
	char *phone_num = NULL;
	while(1){
		if ((phone_num = atcmd_get_phonenum()) != NULL)
		{
			gmmp_set_auth_id(phone_num);
			break;
		}
		else{
			sleep(60);
		}
	}
	// CIP server registration
	send_device_registration();
#else
	gmmp_set_auth_id("01020960200");
#endif

	//GMMP Module Init�� �ȵǸ� Critical�� �����̹Ƿ� ���� �ݺ��Ѵ�.
	while(1){
		ret = GMMP_Module_Init();
		if(ret == 0) {
			break;
		}
		else {
			printf("GMMP Gateway Init Error : ret[%d]\n", ret);
			sleep(60);
		}
	}
#ifndef TACOC_STANDALONE
	// sending CIP regi data using gmmp push.
	send_dtg_etc_packet_to_gmmp(MSG_TYPE_REGISTRATION);
#endif
	init_delivery_timer();
	set_delivery_timer();
	
#ifndef TACOC_STANDALONE
	taco_request_call_wrapper();
#else
#ifdef SMS_TEST
	ret = init_uart("/dev/smd25",115200, &g_modem_uart_fd);
	if( ret < 0 )
	{
		printf("init_modem_uart> fail!\n");
		return -1;
	}
	pthread_create(&tid_sms_thread, NULL, sms_thread, NULL);
#endif
	if(argc>=2){
		char smstestbuf[90];
		if(strcmp(argv[1],"-h")==0)
		{
			printf("example) test_gmmp -s smsdata\n");
		}
		else if(strcmp(argv[1],"-s")==0)
		{
			memset(smstestbuf,0,90);
			strncpy(smstestbuf,argv[2],89);
			printf("[sms data parameter] [%s]\n",smstestbuf);
			GMMP_Control_Process(smstestbuf,strlen(smstestbuf),SMS);
		}
	}
#endif

	while(1){
		sleep(1);
	}
	return 0;
}

static void delivery_handler(int sig, siginfo_t *si, void *uc)
{
	int result;
	if(gmmp_get_delivery_state()==0){
		return;
	}
#ifndef TACOC_STANDALONE
	taco_request_call_wrapper();
#elif 1
	unsigned char testdata[]="data";
	GMMP_Delivery_Packet(COLLECT_DATA,0x01,testdata,strlen(testdata));
	set_delivery_timer();
#elif 0
	unsigned char *testdata;
	testdata=malloc(2048);
	memset(testdata,'b',2048);
	memset(testdata,'a',1024);
	GMMP_Delivery_Packet(COLLECT_DATA,MEDIA_TYPE_MSG_HTTP,testdata,2048);
	free(testdata);
	set_delivery_timer();
#endif
}

struct sigaction sa;
struct sigevent sev;
timer_t tid_delivery;
struct itimerspec ts;
int init_delivery_timer(void)
{
	int reporting_period;

	reporting_period = gmmp_get_reporting_period();
	if(reporting_period == 0){
		return 0;
	}

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = delivery_handler;
	sigemptyset(&sa.sa_mask);
	
	if (sigaction(SIGRTMAX, &sa, NULL) == -1) {
		printf("sigaction error\n");
		return -1;
	}
	
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGRTMAX;
	
	//��ȸ�� timer�� ������ ���츦 ������ interval. ���� ���ϴ� ������ taco���� ����Ÿ�� �ִ��� ���� �о��ö��� �ð�(�ִ�60��)�� �ݿ��Ǿ��� �Ѵ�.
	//���۽ð��� �ݿ��Ǿ�����. ���� 3g ���� ���ε� �ӵ��� �ּ� 51KB���� ���� �����ȵ� xml����Ÿ�� ũ���� 111*3000��+157=333157. �뷫 6.5��. 10������.
	//�׸��� ��Ʈ��ũ�� ���۽� timeout�� �ݿ��Ǵ� �ð��� 30��- �˳��ϰ� �� 100�ʷ� �ؾ��Ұ����� ����.
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = reporting_period*60;
	if(ts.it_value.tv_sec<6)
		ts.it_value.tv_sec=6;
	ts.it_value.tv_nsec = 0;
	
	sev.sigev_value.sival_ptr = &tid_delivery;
	
	timer_create(CLOCK_REALTIME, &sev, &tid_delivery);
	return 0;
}

int set_delivery_timer(void)
{
	timer_settime(tid_delivery,0,&ts,NULL);

	return 0;
}

