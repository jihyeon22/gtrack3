
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <board/power.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include "dtg_gtrack_tool.h"

#include <wrapper/dtg_atcmd.h>
#include <wrapper/dtg_version.h>
#include <board/modem-time.h>
#include <board/board_system.h>

#include "dtg_carnum_tool.h"

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// putty : ISO-8859-1:1998 (Latin-1, West Europe)
// text editor : ansi encoding
// ---------------------------------------------------------------------------------------

#define MAX_CAR_NUM_DEVIDE	4


#define IS_NUMBER	0
#define IS_STRING	1


typedef struct
{
    char devide_str[MAX_CAR_NUM_DEVIDE][64];
}CAR_NUM_DEVIDE_T;

typedef struct
{
    char * cmd;
	int code;
}REGI_CODE_TABLE_T;

static REGI_CODE_TABLE_T car_num_regi_code_table[] =
{
	{"없음", 0}, {"강원", 9},
	{"서울", 1}, {"충북", 10},
	{"부산", 2}, {"충남", 11},
	{"대구", 3}, {"전북", 12},
	{"인천", 4}, {"전남", 13},
	{"광주", 5}, {"경북", 14},
	{"대전", 6}, {"경남", 15},
	{"울산", 7}, {"제주", 16},
	{"경기", 8}, {"세종", 17},
	{NULL, 0},
};

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

static REGI_CODE_TABLE_T car_num_middle_code_table[] =
{
	{"가",0},   {"로",17},  {"아",34},
	{"나",1},   {"모",18},  {"자",35},
	{"다",2},   {"보",19},  {"허",36},
	{"라",3},   {"소",20},  {"외교",37},
	{"마",4},   {"오",21},  {"영사",38},
	{"거",5},   {"조",22},  {"준외",39},
	{"너",6},   {"구",23},  {"준영",40},
	{"더",7},   {"누",24},  {"국기",41},
	{"러",8},   {"두",25},  {"협정",42},
	{"머",9},   {"루",26},  {"대표",43},
	{"버",10},  {"무",27},  {"육",44},
	{"서",11},  {"부",28},  {"공",45},
	{"어",12},  {"수",29},  {"해",46},
	{"저",13},  {"우",30},  {"어",47},
	{"고",14},  {"주",31},  {"배",48},
	{"노",15},  {"바",32},  {"하",49},
	{"도",16},  {"사",33},  {"호",50},
	{NULL, 0},
};

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

static REGI_CODE_TABLE_T car_type_code_table[] =
{
    {"11", 0}, //: 시내버스 	0
    {"12", 1}, //: 농어촌버스 	1
    {"13", 2}, //: 마을버스 	2
    {"14", 3}, //: 시외버스 	3
    {"15", 4}, //: 고속버스 	4
    {"16", 5}, //: 전세버스 	5
    {"17", 6}, //: 특수여객자동차 	6
    {"21", 7}, //: 일반택시 	7
    {"22", 8}, //: 개인택시 	8
    {"31", 9}, //: 일반화물자동차 	9
    {"32", 0}, //: 개별화물자동차 	10
    {"41", 1}, //: 비사업용자동차 	11
	{NULL, 0},
};
// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

/*
int lila_dtg__kor_test()
{
	printf("???????????.\r\n");
	printf("??????? sizeof -> [%d]\r\n", sizeof("??"));
}*/

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

int lila_dtg__convert_car_num_regi_code(char* reg_str, int size) // 4byte
{
	int i = 0;
	int ret_code = 0;
	
	while(car_num_regi_code_table[i].cmd != NULL)
	{
		if( memcmp(reg_str, car_num_regi_code_table[i].cmd, size) == 0 )
		{
			ret_code = car_num_regi_code_table[i].code;
			// printf("lila_dtg__convert_car_num_regi_code success!!! [%s] [%d]\r\n", car_num_regi_code_table[i].cmd, car_num_regi_code_table[i].code);
			break;
		}
		i++;
	}
	return ret_code;
}

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

int lila_dtg__convert_car_num_middle_code(char* reg_str, int size) // 4byte
{
	int i = 0;
	int ret_code = 0;
	
	while(car_num_middle_code_table[i].cmd != NULL)
	{
		if( memcmp(reg_str, car_num_middle_code_table[i].cmd, size) == 0 )
		{
			ret_code = car_num_middle_code_table[i].code;
			// printf("lila_dtg__convert_car_num_middle_code success!!! [%s] [%d]\r\n", car_num_middle_code_table[i].cmd, car_num_middle_code_table[i].code);
			break;
		}
		i++;
	}
	return ret_code;
}

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

int lila_dtg__onvert_car_type_code(char* reg_str, int size) // 4byte
{
	int i = 0;
	int ret_code = 0;
	
	while(car_type_code_table[i].cmd != NULL)
	{
		if( memcmp(reg_str, car_type_code_table[i].cmd, size) == 0 )
		{
			ret_code = car_type_code_table[i].code;
			// printf("lila_dtg__onvert_car_type_code success!!! [%s] [%d]\r\n", car_type_code_table[i].cmd, car_type_code_table[i].code);
			break;
		}
		i++;
	}
	return ret_code;
}


// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

int _get_is_number(char ch)
{
	if ( ( ch >= '0' ) && ( ch <= '9' ) )
	{
		return IS_NUMBER;
	}
	else
	{
		return IS_STRING;
	}
}

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

int lila_dtg__car_num_devide(char* buff, int size, CAR_NUM_DEVIDE_T* p_car_num )
{
	int i = 0;
	int j = 0;
	int devide_idx = 0;
	
	int cur_stat = IS_STRING;
	int last_stat = IS_STRING;
	
	for( i = 0 ; i < size ; i++ )
	{
		cur_stat = _get_is_number(buff[i]);
		if ( cur_stat != last_stat )
		{
			devide_idx++;
			j = 0;
		}
		last_stat = cur_stat;
		
		if (devide_idx > MAX_CAR_NUM_DEVIDE)
			return -1;
		
		p_car_num->devide_str[devide_idx][j++] = buff[i];
	}
	return 0;
}

// ---------------------------------------------------------------------------------------
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// only edit use to notepad++ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   -> ansi encoding
//   -> ansi file sped
// ---------------------------------------------------------------------------------------

char _get_number_of_digit(int num, int digit)
{
	int tmp_num_1 = num;
	int tmp_num_2 = num;
	
	int ret_val = 0;
	
	tmp_num_1 = num % (digit*10);
	tmp_num_2 = num % (digit);
	
	ret_val = (tmp_num_1 - tmp_num_2) / digit;
	
	// printf("_get_number_of_digit :: input [%d] / digit [%d] => ret [%d]\r\n", num, digit, ret_val);
	return ret_val;
}

char _convert_number_to_bit_field(int num)
{
	char tmp_digit[2] = {0,};
	char ret = 0;
	
	tmp_digit[0] = _get_number_of_digit(num, 10);
	tmp_digit[1] = _get_number_of_digit(num, 1);
	
	ret += tmp_digit[0] * 0x10;
    ret += tmp_digit[1] * 0x1;
	
	return ret;
}


int lila_dtg__convert_car_num(tacom_std_hdr_t* p_current_std_hdr, char* convert_buff )
{
	CAR_NUM_DEVIDE_T car_num;
	
	int car_type_code = 0;
	int car_num_region_code_code = 0;
	int car_num_1 = 0;
	int car_num_middle_code = 0;
	int car_num_2 = 0;
	
	char temp_res_buff[12] = {0,};
	
	printf("input car num is [%s]\r\n", p_current_std_hdr->registration_num);
	printf("input car num is [%s]\r\n", p_current_std_hdr->registration_num);
	printf("input car num is [%s]\r\n", p_current_std_hdr->registration_num);
	
	memset(&car_num, 0x00, sizeof(car_num));
	
	lila_dtg__car_num_devide(p_current_std_hdr->registration_num, 12, &car_num);
	
	{
		int i = 0;
		for ( i = 0 ; i < 4 ; i ++)
		{
			printf("devide result [%s]\r\n", car_num.devide_str[i]);
		}
	}
	
	
	car_type_code = lila_dtg__onvert_car_type_code(p_current_std_hdr->vehicle_type,2);
	//printf("car_type_code is [%d]\r\n", car_type_code);
	car_num_region_code_code = lila_dtg__convert_car_num_regi_code(car_num.devide_str[0], strlen(car_num.devide_str[0]));
	//printf("car_num_region_code_code is [%d]\r\n", car_num_region_code_code);
	car_num_1 = atoi(car_num.devide_str[1]);
	//printf("car_num_1 is [%d]\r\n", car_num_1);
	car_num_middle_code = lila_dtg__convert_car_num_middle_code(car_num.devide_str[2], strlen(car_num.devide_str[2]));
	//printf("car_num_middle_code is [%d]\r\n", car_num_middle_code);
	car_num_2 = atoi(car_num.devide_str[3]);
	//printf("car_num_2 is [%d]\r\n", car_num_2);
	
	
	temp_res_buff[0] = _convert_number_to_bit_field(car_type_code); // ????????
	
	temp_res_buff[1] = _convert_number_to_bit_field(car_num_region_code_code); // ???????
	
	temp_res_buff[2] = _convert_number_to_bit_field( _get_number_of_digit(car_num_1, 10) );// ??????1 - 10???
	temp_res_buff[3] = _convert_number_to_bit_field( _get_number_of_digit(car_num_1, 1) );// ??????1 - 1???
	
	temp_res_buff[4] =  car_num_middle_code;// ???????? - 1
	
	temp_res_buff[5] = _convert_number_to_bit_field( _get_number_of_digit(car_num_2, 1000));
	temp_res_buff[6] = _convert_number_to_bit_field( _get_number_of_digit(car_num_2, 100));
	temp_res_buff[7] = _convert_number_to_bit_field( _get_number_of_digit(car_num_2, 10));
	temp_res_buff[8] = _convert_number_to_bit_field( _get_number_of_digit(car_num_2, 1));
	
	temp_res_buff[9] = 0;
	temp_res_buff[10] = 0;
	temp_res_buff[11] = 0;
	
	
	/*
	printf("\r\n ------ convert car num ------------------------ \r\n");
	for ( i = 0 ; i < 12; i++)
	{
		printf("[%d]", temp_res_buff[i]);
	}
	printf("\r\n ------ convert car num ------------------------ \r\n");
	*/
	memcpy(convert_buff, &temp_res_buff, 12);
	return 0;
}
