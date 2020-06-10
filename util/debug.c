<<<<<<< HEAD
#include <stdio.h>

void debug_hexdump_buff(unsigned char *buff, const int buff_len)
{
	int i;

	printf("dump buf\n");
	for(i = 0; i < buff_len; i++)
	{
		printf("0x%02x ", buff[i]);
	}
	printf("\n");
}

=======
#include <stdio.h>

void debug_hexdump_buff(unsigned char *buff, const int buff_len)
{
	int i;

	printf("dump buf\n");
	for(i = 0; i < buff_len; i++)
	{
		printf("0x%02x ", buff[i]);
	}
	printf("\n");
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
