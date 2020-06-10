 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <time.h>
 #include <errno.h>

 #include <stdlib.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <string.h>
 #include <termios.h>
 #include <fcntl.h>
 #include <sys/time.h>


int init_uart(const char* dev, const int baud)
{
	struct termios newtio;
	int fd = 0;

	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	//fd = open(dev, O_RDWR | O_NOCTTY);

	if(fd < 0) {
		return -1;
	}

	int val;
	val = fcntl(fd, F_GETFD, 0);
	val |= FD_CLOEXEC;
	fcntl(fd, F_SETFD, val);

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts

	switch(baud)
	{
		case 115200 :
			newtio.c_cflag |= B115200;
			break;
		case 57600 :
			newtio.c_cflag |= B57600;
			break;
		case 38400 :
			newtio.c_cflag |= B38400;
			break;
		case 19200 :
			newtio.c_cflag |= B19200;
			break;
		case 9600 :
			newtio.c_cflag |= B9600;
			break;
		case 4800 :
			newtio.c_cflag |= B4800;
			break;
		case 2400 :
			newtio.c_cflag |= B2400;
			break;
		default :
			newtio.c_cflag |= B115200;
			break;
	}



	newtio.c_lflag = 0;
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1초 단위
	//newtio.c_cc[VMIN] = vmin; // 최소 n 문자 받을 때까진 대기
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	return fd;
}
int at_bridge_modem_fd = 0;
#define AT_MAX_BUFF_SIZE  512

int atb_write_modem(const char *buff, int buff_len)
{
	int ret;
	ret = write(at_bridge_modem_fd, buff, buff_len);
	printf("write ret is [%d]\r\n",ret);
	return ret;
}

char test_cmd[] ="GET /FWDL/XDT1000TFMS.bin HTTP/1.1\r\nHost: 219.254.35.204\r\nConnection: close\r\nConnection: keep-alive\r\nUser-Agent: XDT-1000/04\r\n\n\n";

void check_op_time(char *msg, struct timeval ot, struct timeval nt)
{
	double op_time;

	op_time = (double)(nt.tv_sec) + (double)(nt.tv_usec)/1000000.0 - (double)(ot.tv_sec) - (double)(ot.tv_usec)/1000000.0;
	printf("%s op time : %f\n", msg, op_time);
}

fd_set reads;
unsigned char g_buf[1024*1024];
static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
	
	struct timeval tout;
	int result = 0;
	int len = 0;
	int uart_len;

	
	while (1) {
		FD_ZERO(&reads);
		FD_SET(fd, &reads);

		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return len;

		uart_len = read(fd, &buf[len], buf_len-len);
		if(uart_len <= 0)
			return len;

		len += uart_len;
		continue; //success
	}

	return len;
}


#define UART0_DEV_NAME					"/dev/ttyHSL1"


static int test_wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;
	int len = 0;
	int uart_len;
	int n_try = 1000;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	tout.tv_sec = ftime;
	tout.tv_usec = 0;
	result = select(fd + 1, &reads, 0, 0, &tout);
	if(result <= 0) //time out & select error
		return len;

	while(n_try-- > 0)
	{
		uart_len = read(fd, &(buf[len]), buf_len-len);
		if(uart_len <= 0)
			break;
		
		len += uart_len;
	}

	return len;
}

int main(int argc, char *argv[])
{
	
	int len = 0;
	char msg[512];
	int baudrate;


	//example : baudrate devname, command
	if(argc != 4) {
		printf("argc is error [%d]\n", argc);
		printf("example baudrate devname, command\n");
		return -1;
	}

	baudrate = atoi(argv[1]);

	printf("ttydev name : [%s]\n", argv[2]);
	printf("baudrate : [%d]\n", baudrate);
	printf("cmd : %s\n", argv[3]);

	
	at_bridge_modem_fd = init_uart(argv[2], baudrate);
	if(at_bridge_modem_fd <= 0) {
		printf("uart dev open error [%d]\n", at_bridge_modem_fd);
		return -1;
	}

	atb_write_modem(argv[3], strlen(argv[3]));

	memset(msg, 0x00, sizeof(msg));
	len = test_wait_read(at_bridge_modem_fd, msg, sizeof(msg), 2);
	printf("len = [%d]\n", len);
	printf("recv msg = [%s]\n", msg);

	return 0;
}
