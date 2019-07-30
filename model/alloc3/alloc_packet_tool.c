#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "alloc_packet_tool.h"
#include "alloc_util.h"
#include "config.h"
#include "buzzer.h"
#include "tagging.h"
#include "geofence.h"

#include <base/gpstool.h>
#include <util/storage.h>
#include "logd/logd_rpc.h"

#define DBG_MSG_SAVE_PASSENGER_INFO
#define DBG_MSG_SAVE_GEOFENCE_INFO

#define LOG_TARGET eSVC_MODEL
// -------------------------------------------------------------------------------
// boyer moore..
// -------------------------------------------------------------------------------

# include <limits.h>

# include "Ftp_ClientCmd.h"

#include <dm/update.h>
#include <dm/update_api.h>
#include <at/at_util.h>

// jhcho test 
#include "color_printf.h"
#include "netcom.h"
#include <base/sender.h>

#include <ctype.h>

# define NO_OF_CHARS 256
 
// -------------------------------------------------------------------------------
// Extern 
// -------------------------------------------------------------------------------
extern int g_rfid_fd;
extern int g_tl500_state;
extern int g_rfid_requestdb;
extern char g_rfid_filename[32];
extern int g_tl500_geofence_reset;   


char _ex_buff[ MAX_RCV_PACKET_SIZE ] = {0,};
int b_ex_buff = 0;

// A utility function to get maximum of two integers
static int max (int a, int b) { return (a > b)? a: b; }
 
// The preprocessing function for Boyer Moore's bad character heuristic
static void badCharHeuristic( const char *str, int size, int badchar[NO_OF_CHARS])
{
    int i;
 
    // Initialize all occurrences as -1
    for (i = 0; i < NO_OF_CHARS; i++)
         badchar[i] = -1;
 
    // Fill the actual value of last occurrence of a character
    for (i = 0; i < size; i++)
         badchar[(int) str[i]] = i;
}
 
/* A pattern searching function that uses Bad Character Heuristic of
   Boyer Moore Algorithm */
int bm_search( const char *txt,  int n, const char *pat, int m)
{
 
    int badchar[NO_OF_CHARS];
 
    /* Fill the bad character array by calling the preprocessing
       function badCharHeuristic() for given pattern */
    badCharHeuristic(pat, m, badchar);
 
    int s = 0;  // s is shift of the pattern with respect to text
    while(s <= (n - m))
    {
        int j = m-1;
 
        /* Keep reducing index j of pattern while characters of
           pattern and text are matching at this shift s */
        while(j >= 0 && pat[j] == txt[s+j])
            j--;
 
 
        /* If the pattern is present at current shift, then index j
           will become -1 after the above loop */
        if (j < 0)
        {
            // printf("\n pattern occurs at shift = %d\r\n", s);
			// kksowrks : return char start index..
			return s;
 
            /* Shift the pattern so that the next character in text
               aligns with the last occurrence of it in pattern.
               The condition s+m < n is necessary for the case when
               pattern occurs at the end of text */
            s += (s+m < n)? m-badchar[(int)(txt[s+m])] : 1;
 
        }
 
        else
            /* Shift the pattern so that the bad character in text
               aligns with the last occurrence of it in pattern. The
               max function is used to make sure that we get a positive
               shift. We may get a negative shift if the last occurrence
               of bad character in pattern is on the right side of the
               current character. */
            s += max(1, j - badchar[(int)(txt[s+j])]);
    }
	
	return -1;
	
}



// -------------------------------------------------------------------------------
// passenger info..
// -------------------------------------------------------------------------------

static allocation_passenger_info_t passenger_info;

void init_passenger_info()
{
	// init status
	passenger_info.status = eGET_PASSENGER_STAT_NULL;
	
	// init cmd id
	memset(&passenger_info.cmd_id, '\0', 10);
	
	// init pkt cnt	
	passenger_info.max_pkt = 0;
	passenger_info.cur_pkt = 0;
	
	// init version
	strncpy((char *)passenger_info.version, "00000000", strlen("00000000") );
	
	// init rfid list
	memset(passenger_info.rfid_list, '\0', sizeof(passenger_info.rfid_list));
	
	// init rfid list len
	passenger_info.rfid_list_len = 0;
}

int load_file_passenger_info(char* version)
{
	allocation_passenger_info_t tmp_passenger_buff = {0,};
	
	if ( storage_load_file(RFID_SAVED_FILE, &tmp_passenger_buff, sizeof(allocation_passenger_info_t)) >= 0)
	{
		// 읽기 성공
		memcpy(&passenger_info, (void*)&tmp_passenger_buff, sizeof(allocation_passenger_info_t));
		strncpy(version, tmp_passenger_buff.version, 8);
	}
	else
	{
		// 읽기 실패 혹은 파일이 없다.
		init_passenger_info();
	}

	load_circulating_bus_info();
	return 0;
}


int set_passenger_info_lastest_ver()
{
	//load_file_passenger_info(NULL, 1);
	
	return passenger_info.status;
}

int get_passenger_info_ver(char* version)
{
	strncpy(version, passenger_info.version, 8);
	return 0;
}

int get_passenger_info_stat()
{
	return passenger_info.status;
}

int get_passenger_cmd_id(char* cmd_id)
{
	strncpy(cmd_id, passenger_info.cmd_id, 10);
	
	return 0;
}

void print_passenger_info()
{
	printf("----------------- passenger info --------------------\r\n");
	printf("passenger_info.status [%d]\r\n",passenger_info.status);
	printf("passenger_info.cmd_id [%s]\r\n",passenger_info.cmd_id);
	printf("passenger_info.max_pkt is [%d] \r\n",passenger_info.max_pkt);
	printf("passenger_info.cur_pkt is [%d] \r\n",passenger_info.cur_pkt);
	printf("passenger_info.version is [%s] \r\n",passenger_info.version);
	printf("passenger_info.rfid_list_len is [%d] \r\n",passenger_info.rfid_list_len);
	printf("passenger_info.rfid_list is [%s]  \r\n",passenger_info.rfid_list);
	printf("------------------------------------------------------\r\n");
}



int save_passenger_info(packet_frame_t result)
{
	char token[ ] = ",";
	char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	
	char tmp_data_buff[512] = {0,};
	
	int err = 0;
	
	char* buff_p = result.packet_content;
	int data_size = 0;
	
	//printf("start insert packet [%s] +++++ \r\n",buff_p);
	
	// check status
	if ( passenger_info.status == eGET_PASSENGER_STAT_COMPLETE )
	{
		printf("start is complete skip insert..\r\n");
		// 만약 기존에 성공이 세팅되어이있었는데, 패킷이 들어왔다면,
		// 기존의 정보를 초기화 한다.
		// 가정은 모든 패킷이 정상적으로 들어온다는 가정!
		init_passenger_info();
		//return passenger_info.status;
	}
	
	// ==============================================
	// 0 : pkt command 
	// ==============================================
	tr = strtok_r(buff_p, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	//printf("   - rfid pkt cmd [%s]\r\n",tr);
	
	// ==============================================
	// 1 : phone num
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -2;
		goto CMD_PARSE_FAIL;
	}
	//printf("   - rfid phone num [%s]\r\n",tr);
	
	// ==============================================
	// 2 : cmd id
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -3;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
		//printf("   - rfid cmd id [%s]\r\n",tr);
		if ( passenger_info.status == eGET_PASSENGER_STAT_NULL )
		{
			memset(&passenger_info.cmd_id, '\0', 10);
			strncpy(passenger_info.cmd_id, tr, strlen(tr));
			//printf("    -> rfid cmd cpy [%s]\r\n",passenger_info.cmd_id);
			
		}
		else if( passenger_info.status == eGET_PASSENGER_STAT_DOWNLOADING ) 
		{
			if ( strncmp((char *)passenger_info.cmd_id, tr, strlen(tr)) != 0 )
			{
				//printf("    -> rfid cmd invalid..\r\n");
				err = -4;
				goto CMD_PARSE_FAIL;
			}
			//printf("    -> rfid cmd already cpy skip..\r\n");
		}
	}
	
	// ==============================================
	// 3 : data size
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -6;
		goto CMD_PARSE_FAIL;
	}
	
	data_size = atoi(tr);
	//printf("   - rfid data_size [%d]\r\n", data_size);
	
	// ==============================================
	// 4 : max pkt
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -7;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
		//printf("   - rfid max pkt [%s]\r\n",tr);
		if ( passenger_info.status == eGET_PASSENGER_STAT_NULL )
		{
			passenger_info.max_pkt = atoi(tr);
			passenger_info.cur_pkt = 0;
			
			passenger_info.status = eGET_PASSENGER_STAT_DOWNLOADING;
		//	printf("    -> rfid max pkt set [%d]\r\n",passenger_info.max_pkt);
		}
		else if (passenger_info.status == eGET_PASSENGER_STAT_DOWNLOADING ) 
		{
			if ( passenger_info.max_pkt != atoi(tr) )
			{
				printf("    -> rfid max pkt invalid\r\n");
				err = -8;
				goto CMD_PARSE_FAIL;
			}
			//printf("    -> rfid max pkt already set.. skip..\r\n");
		}
		
	}
	
	// ==============================================
	// 5 : cur pkt
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -9;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
		//printf("   - rfid cur pkt [%s]\r\n",tr);
		if ( passenger_info.status == eGET_PASSENGER_STAT_NULL )
		{
			//printf("    -> rfid cur pkt : not download mode... fail\r\n");
			goto CMD_PARSE_FAIL;
		}
		else if (passenger_info.status == eGET_PASSENGER_STAT_DOWNLOADING ) 
		{
			int cur_pkt_idx = atoi(tr);
			int target_pkt_idx = passenger_info.cur_pkt+1;
			
			//printf("         passenger_info.cur_pkt [%d] / cur_pkt_idx [%d]\r\n", target_pkt_idx, cur_pkt_idx);
			// check current index
			if ( target_pkt_idx != cur_pkt_idx)
			{
				printf("    -> rfid cur pkt : pkt compare fail.. \r\n");
				err = -9;
				goto CMD_PARSE_FAIL;
			}
			
			passenger_info.cur_pkt = cur_pkt_idx;
			//printf("    -> rfid cur pkt : set [%d] \r\n", passenger_info.cur_pkt);
			
			// check complete..
			if (  passenger_info.max_pkt == passenger_info.cur_pkt )
			{
				// passenger_info.status = eGET_PASSENGER_STAT_COMPLETE;
			}
		}
	}
	
	// ==============================================
	// 6~7 : get total data frame
	// ==============================================
	tr = strtok_r(NULL, token_1, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -10;
		goto CMD_PARSE_FAIL;
	}
	
	//printf("   - check data field... \r\n");
	
	if(1) //....
	{
		if ( data_size != strlen(tr) )
		{
			printf("    -> check data field : data len invalid (%d/%d) \r\n", data_size, strlen(tr));
			err = -10;
			goto CMD_PARSE_FAIL;
		}
		
		//printf("    -> check data field : data len (%d/%d) \r\n", data_size, strlen(tr));
		
		memset(tmp_data_buff, 0x00, 512);
		strncpy(tmp_data_buff, tr, strlen(tr));
	}
	
	// ==============================================
	// 6 : data version
	// ==============================================
	tr = strtok_r(tmp_data_buff, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -10;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
		//printf("   - rfid data ver [%s]\r\n",tr);
		
		if ( strncmp((char *)passenger_info.version, "00000000", strlen("00000000")) == 0)
		{
			strncpy(passenger_info.version, tr, strlen(tr) );
		//	printf("    -> rfid data ver [%d] \r\n", passenger_info.version);
		
		}
		else if ( strncmp((char *)passenger_info.version, tr, strlen(tr)) != 0 )
		{
			err = -13;
			printf("    -> rfid data ver invalid \r\n");
			goto CMD_PARSE_FAIL;
		}
			
		
	}
	
	// ==============================================
	// 7 : data
	// ==============================================
	tr = strtok_r(NULL, token_1, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -10;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
		//printf("   - rfid data pkt [%s]\r\n",tr);
		
		//strncpy(tmp_data_buff, tr, strlen(tr));
		
		strncpy(passenger_info.rfid_list + passenger_info.rfid_list_len, tr, strlen(tr));
		passenger_info.rfid_list_len += strlen(tr);
		
		// last data spilt
		passenger_info.rfid_list[passenger_info.rfid_list_len] = ',';
		passenger_info.rfid_list_len += 1 ;
		
		//printf("    -> rfid cur pkt : rfid_list_len [%d] \r\n", passenger_info.rfid_list_len);
		//printf("    -> rfid cur pkt : set [%s] \r\n", passenger_info.rfid_list);
		
		if ( passenger_info.max_pkt == passenger_info.cur_pkt )
		{
			//printf("    -> rfid cur pkt : set [%s] \r\n", passenger_info.rfid_list);
			passenger_info.status = eGET_PASSENGER_STAT_COMPLETE;
			storage_save_file(RFID_SAVED_FILE, (void*)&passenger_info, sizeof(allocation_passenger_info_t));
		}
	
	}
	
CMD_PARSE_FAIL:
	if (err < 0)
	{
		init_passenger_info();
	}
	
	return passenger_info.status;
}




int find_rfid(char* rfid_pat, int size, char *rfid_date)
{
	int ret = 0;

	ret = bm_search((const char *)passenger_info.rfid_list, passenger_info.rfid_list_len ,rfid_pat, size);
	
	//tagging_add_rfid(rfid_pat, get_recent_geo_fence(), rfid_date);
	if (ret >= 0 )
	{
		printf("rfid found!!!! [%s] / [%d]\r\n", rfid_pat, size);
		tagging_add_rfid(rfid_pat, get_recent_geo_fence(), rfid_date);
		buzzer_run(BUZZER1, 1, 500, 0);
	}
	else
	{
		printf("rfid not found!!!! NO!!!!!!!!!!!!!!! [%s] / [%d]\r\n", rfid_pat, size);
#if SEND_NOT_EXIST_RFID
		tagging_add_rfid(rfid_pat, get_recent_geo_fence(), rfid_date);
#endif
		buzzer_run(BUZZER2, 1, 1000, 0);

	}
		
	return ret;
}


// -------------------------------------------------------------------------------
// geofence info..
// -------------------------------------------------------------------------------
#include "geofence.h"
static allocation_geofence_info_t geo_fence_info;

void alloc_geo_fence_info_init()
{
	memset(&geo_fence_info, 0x00, sizeof(allocation_geofence_info_t));
	init_geo_fence(eGEN_FENCE_DEBUG_MODE); //jwrho
}



int load_file_geofence_info()
{
	allocation_geofence_info_t tmp_geofence_buff = {0,};
	
	if ( storage_load_file(GEOFENCE_SAVED_FILE, &tmp_geofence_buff, sizeof(allocation_geofence_info_t)) >= 0)
	{
		// 읽기 성공
		memcpy(&geo_fence_info, (void*)&tmp_geofence_buff, sizeof(allocation_geofence_info_t));
		
	//	set_geo_fence_setup_info(i, &geo_fence_data);
	
		// 매번 부팅시마다 일단 세팅한다.???
		set_geofence_data();
		set_recent_geo_fence(geo_fence_info.recent_geo_fence);		
	//	strncpy(version, tmp_passenger_buff.version, 8);
	}
	else
	{
		// 읽기 실패 혹은 파일이 없다.
		LOGE(LOG_TARGET, "GEOFENCE LOAD FAIL... : saved file not found\n");
		alloc_geo_fence_info_init();
	}

	
	return 0;
}

int save_geofence_info(packet_frame_t result)
{
	char token[ ] = ",";
	char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	
	char tmp_data_buff[512] = {0,};
	
	int err = 0;	

	char* buff_p = (char *)result.packet_content;
	int data_size = 0;
	
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
	printf("save Genfence : start [%s] +++++ \r\n",buff_p);
#endif

	// check status
	if ( geo_fence_info.status == eGET_GEOFENCE_STAT_COMPLETE )
	{
		printf("save Genfence : complete status skip insert..\r\n");
		// 기존것을 지우고 새로 받는다는 이야기
		alloc_geo_fence_info_init();
		//return geo_fence_info.status;

	}
	
	// ==============================================
	// 0 : pkt command 
	// ==============================================
	tr = strtok_r(buff_p, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
	printf("   - Genfence pkt cmd [%s]\r\n",tr);
#endif

	// ==============================================
	// 1 : phone num
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -2;
		goto CMD_PARSE_FAIL;
	}
	
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
	printf("   - Genfence phone num [%s]\r\n",tr);
#endif	

	// ==============================================
	// 2 : cmd id
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -3;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
		printf("   - Genfence cmd id [%s]\r\n",tr);
#endif
		if ( geo_fence_info.status == eGET_GEOFENCE_STAT_NULL )
		{
			memset(&geo_fence_info.cmd_id, '\0', 10);
			strncpy((char *) geo_fence_info.cmd_id, tr, strlen(tr));
			//printf("    -> Genfence cmd cpy [%s]\r\n",geo_fence_info.cmd_id);
			
		}
		else if( geo_fence_info.status == eGET_GEOFENCE_STAT_DOWNLOADING ) 
		{
			if ( strncmp(geo_fence_info.cmd_id, tr, strlen(tr)) != 0 )
			{
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
				printf("    -> Genfence cmd invalid..\r\n");
#endif
				err = -4;
				goto CMD_PARSE_FAIL;
			}
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("    -> Genfence cmd already cpy skip..\r\n");
#endif
		}
	}
	
	// ==============================================
	// 3 : data size
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -6;
		goto CMD_PARSE_FAIL;
	}
	
	data_size = atoi(tr);
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
	printf("   - Geofence data_size [%d]\r\n", data_size);
#endif
	// ==============================================
	// 4 : max pkt
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -7;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
		printf("   - GeoFence max pkt [%s]\r\n",tr);
#endif
		if ( geo_fence_info.status == eGET_GEOFENCE_STAT_NULL )
		{
			geo_fence_info.max_pkt = atoi(tr);
			geo_fence_info.cur_pkt = 0;
			
			geo_fence_info.status = eGET_PASSENGER_STAT_DOWNLOADING;
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("    -> GeoFence max pkt set [%d]\r\n",geo_fence_info.max_pkt);
#endif
		}
		else if (geo_fence_info.status == eGET_GEOFENCE_STAT_DOWNLOADING ) 
		{
			if ( geo_fence_info.max_pkt != atoi(tr) )
			{
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
				printf("    -> geo_fence_info max pkt invalid\r\n");
#endif
				err = -8;
				goto CMD_PARSE_FAIL;
			}
			//printf("    -> rfid max pkt already set.. skip..\r\n");
		}
		
	}
	
	// ==============================================
	// 5 : cur pkt
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -9;
		goto CMD_PARSE_FAIL;
	}
	
	if (1) // ..........
	{
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
		printf("   - Geofence cur pkt [%s]\r\n",tr);
#endif
		if ( geo_fence_info.status == eGET_GEOFENCE_STAT_NULL )
		{
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("    -> GeoFence cur pkt : not download mode... fail\r\n");
#endif
			goto CMD_PARSE_FAIL;
		}
		else if (geo_fence_info.status == eGET_GEOFENCE_STAT_DOWNLOADING ) 
		{
			int cur_pkt_idx = atoi(tr);
			int target_pkt_idx = geo_fence_info.cur_pkt+1;
			
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("         geo_fence_info.cur_pkt [%d] / cur_pkt_idx [%d]\r\n", target_pkt_idx, cur_pkt_idx);
#endif

// 			// check current index
// 			if ( target_pkt_idx != cur_pkt_idx)
// 			{
// #ifdef DBG_MSG_SAVE_GEOFENCE_INFO
// 				printf("    -> Geofence cur pkt : pkt compare fail.. \r\n");
// #endif
// 				err = -9;
// 				goto CMD_PARSE_FAIL;
// 			}
			
			geo_fence_info.cur_pkt = cur_pkt_idx;
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("    -> Geofence cur pkt : set [%d] \r\n", geo_fence_info.cur_pkt);
#endif

			// check complete..
			if (  geo_fence_info.max_pkt == geo_fence_info.cur_pkt )
			{
				// geo_fence_info.status = eGET_GEOFENCE_STAT_COMPLETE;
				// skip... for data save
			}
		}
	}
	
	// ==============================================
	// 6~7 : get total data frame
	// ==============================================
	tr = strtok_r(NULL, token_1, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -10;
		goto CMD_PARSE_FAIL;
	}
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
	printf("   - check data field... \r\n");
#endif
	
	if(1) //....
	{
		if ( data_size != strlen(tr) )
		{
			printf("    -> check data field : data len invalid (%d/%d) \r\n", data_size, strlen(tr));
			err = -10;
			goto CMD_PARSE_FAIL;
		}
		
#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
		printf("    -> check data field : data len (%d/%d) \r\n", data_size, strlen(tr));
#endif
		
		memset(tmp_data_buff, 0x00, 512);
		strncpy(tmp_data_buff, tr, strlen(tr));
	}
	
	

	
	
	tr = strtok_r(tmp_data_buff, token, &temp_bp);
	
	while((tr != NULL))
	{
		// 데이터의 끝까지 루핑을 돌면서 저장을 한다.

#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
		printf("Geofence Save idx : [%d]\r\n", geo_fence_info.total_geo_fence);
#endif
		// ==============================================
		// 6 : 정류장번호
		// ==============================================
		if ( (tr == NULL) )
		{
			err = -10;
			goto CMD_PARSE_FAIL;
		}

		if (1) // ..........
		{
	#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("   - Geofence id [%s]\r\n", tr);
	#endif
			geo_fence_info.stop_num[geo_fence_info.total_geo_fence] = atoi(tr);
			
		}
		
		// ==============================================
		// 7 : 위도
		// ==============================================
		tr = strtok_r(NULL, token, &temp_bp);
		if ( (tr == NULL) )
		{
			err = -10;
			goto CMD_PARSE_FAIL;
		}
		
		if (1) // ..........
		{
	#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("   - Geofence lattitude [%s]\r\n", tr);
	#endif
			geo_fence_info.latitude[geo_fence_info.total_geo_fence] = atof(tr);
			
		}
		
		// ==============================================
		// 8 : 경도
		// ==============================================
		tr = strtok_r(NULL, token, &temp_bp);
		if ( (tr == NULL) )
		{
			err = -10;
			goto CMD_PARSE_FAIL;
		}
		
		if (1) // ..........
		{
	#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("   - Geofence longitude [%s]\r\n", tr);
	#endif
			geo_fence_info.longitude[geo_fence_info.total_geo_fence] = atof(tr);
			
		}
		
		// ==============================================
		// 9 : 범위
		// ==============================================
		tr = strtok_r(NULL, token, &temp_bp);
		if ( (tr == NULL) )
		{
			err = -10;
			goto CMD_PARSE_FAIL;
		}

		if (1) // ..........
		{
	#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
			printf("   - Geofence range [%s]\r\n", tr);
	#endif
			geo_fence_info.range[geo_fence_info.total_geo_fence] = atof(tr);
			
		}
		geo_fence_info.total_geo_fence++;
		
		tr = strtok_r(NULL, token, &temp_bp);
	}
	
		
	if (  geo_fence_info.max_pkt == geo_fence_info.cur_pkt )
	{
	#ifdef DBG_MSG_SAVE_GEOFENCE_INFO
		printf("Geofence Save Complete!!!\r\n");
	#endif
		geo_fence_info.status = eGET_GEOFENCE_STAT_COMPLETE;

		printf("geo_fence_info.recent_geo_fence1 :  %d \n", geo_fence_info.recent_geo_fence);
		// 정류장  in 하기 전 까지는 정류장 정보를 기억 하지 않도록 수정
		//geo_fence_info.recent_geo_fence = get_recent_geo_fence();

		set_recent_geo_fence(-1); 
		geo_fence_info.recent_geo_fence = -1;//get_recent_geo_fence();

		printf("geo_fence_info.recent_geo_fence 2:  %d \n", geo_fence_info.recent_geo_fence);		
		
		g_tl500_geofence_reset = 1;

		storage_save_file(GEOFENCE_SAVED_FILE, (void*)&geo_fence_info, sizeof(allocation_geofence_info_t));
		system("sync &");

		set_geofence_data();
		// skip... for data save
	}
	
	
CMD_PARSE_FAIL:
	if (err < 0)
	{
		alloc_geo_fence_info_init();
		// jhcho busstop test [[
		// alloc_evt_pkt_info_t evt_info;
		// gpsData_t gpsdata;

		// memset(&evt_info, 0, sizeof(evt_info));
		
		// gps_get_curr_data(&gpsdata);
		// evt_info.gpsdata = gpsdata;
		// evt_info.diff_distance =  mileage_get_m() * 100;
		
		// mileage_set_m(0);
		
		// print_yellow("err : %d\n", err);
		// if(err == -10)
		// 	err = -3;
		
		// // if(err == -3)
		// // {
		// // 	FILE *out;
		// // 	out = fopen("test.txt", "w"); 

		// // 	fprintf(out, "%s", temp_bp2); 

		// // 	fclose(out); 
		// // }

		// evt_info.evt_code = -err + '0';
		// print_yellow("evt_info.evt_code : %d\n", evt_info.evt_code);
		// sender_add_data_to_buffer(PACKET_TYPE_EVENT, &evt_info, ePIPE_1);
		// jhcho busstop test ]]
	}
	
	return geo_fence_info.status;
}

int get_geo_fence_info_stat()
{
	return geo_fence_info.status;
}

int get_geo_fence_id(int idx, char* geo_fence_id)
{
	sprintf(geo_fence_id, "%d", geo_fence_info.stop_num[idx]);
	//strncpy(geo_fence_id, geo_fence_info.stop_num[idx], strlen(geo_fence_info.stop_num[idx]));
	return 0;
}

int get_geo_fence_cmd_id(char* cmd_id)
{
	strncpy(cmd_id, geo_fence_info.cmd_id, 10);

	return 0;
}

int set_geofence_data()
{
	int i = 0;
	
	if ( geo_fence_info.status != eGET_GEOFENCE_STAT_COMPLETE)
	{
		LOGE(LOG_TARGET, "SET GEOFENCE FAIL !!!! \n");
		return -1;
	}
	
	
	init_geo_fence(eGEN_FENCE_DEBUG_MODE); //jwrho

	for (i = 0 ; i < geo_fence_info.total_geo_fence ; i++)
	{
		geo_fence_setup_t geo_fence_data = {0,};
		
		geo_fence_data.latitude = geo_fence_info.latitude[i];
		geo_fence_data.longitude = geo_fence_info.longitude[i];
		geo_fence_data.range = geo_fence_info.range[i];
		geo_fence_data.setup_fence_status = eFENCE_SETUP_ENTRY_EXIT;
		geo_fence_data.enable = eGEN_FENCE_ENABLE;
		
		LOGI(LOG_TARGET, "SET GEOFENCE : [%d] lat [%f] / lon [%f]\n", i, geo_fence_data.latitude, geo_fence_data.longitude);
		
		set_geo_fence_setup_info(i, &geo_fence_data);
	}

	return 0;
}

void print_geo_fence_info()
{
	int i = 0;
	printf("----------------- print_geo_fence_info info --------------------\r\n");
	printf("geo_fence_info.status [%d]\r\n",geo_fence_info.status);
	printf("geo_fence_info.cmd_id [%s]\r\n",geo_fence_info.cmd_id);
	printf("geo_fence_info.max_pkt is [%d] \r\n",geo_fence_info.max_pkt);
	printf("geo_fence_info.cur_pkt is [%d] \r\n",geo_fence_info.cur_pkt);
	printf("geo_fence_info.total_geo_fence is [%d] \r\n",geo_fence_info.total_geo_fence);

	for ( i = 0 ; i < geo_fence_info.total_geo_fence ; i ++)
	{
		printf("geo_fence_info.stop_num is [%d] \r\n",geo_fence_info.stop_num[i]);
		printf("geo_fence_info.latitude is [%f] \r\n",geo_fence_info.latitude[i]);
		printf("geo_fence_info.longitude is [%f] \r\n",geo_fence_info.longitude[i]);
		printf("geo_fence_info.range is [%d] \r\n",geo_fence_info.range[i]);
	}
	printf("------------------------------------------------------\r\n");
}

int alloc_geo_fence_info_load()
{
#if 0
	int i = 0 ;
	geo_fence_info.stop_num[ geo_fence_info.total_geo_fence ] = 1111;
	geo_fence_info.latitude[ geo_fence_info.total_geo_fence ] = 37.399821;
	geo_fence_info.longitude [ geo_fence_info.total_geo_fence ] = 127.101519;
	geo_fence_info.range[ geo_fence_info.total_geo_fence ] = 100;
	geo_fence_info.total_geo_fence++;
	
	geo_fence_info.stop_num[ geo_fence_info.total_geo_fence ] = 444;
	geo_fence_info.latitude[ geo_fence_info.total_geo_fence ] = 37.394800;
	geo_fence_info.longitude [ geo_fence_info.total_geo_fence ] = 127.105499;
	geo_fence_info.range[ geo_fence_info.total_geo_fence ] = 50;
	geo_fence_info.total_geo_fence++;
	
	geo_fence_info.stop_num[ geo_fence_info.total_geo_fence ] = 666;
	geo_fence_info.latitude[ geo_fence_info.total_geo_fence ] =  37.387955;
	geo_fence_info.longitude [ geo_fence_info.total_geo_fence ] = 127.100446;
	geo_fence_info.range[ geo_fence_info.total_geo_fence ] = 100;
	geo_fence_info.total_geo_fence++;
	
	geo_fence_info.stop_num[ geo_fence_info.total_geo_fence ] = 777;
	geo_fence_info.latitude[ geo_fence_info.total_geo_fence ] = 37.389609;
	geo_fence_info.longitude [ geo_fence_info.total_geo_fence ] = 127.098536;
	geo_fence_info.range[ geo_fence_info.total_geo_fence ] = 100;
	geo_fence_info.total_geo_fence++;
	
	geo_fence_info.stop_num[ geo_fence_info.total_geo_fence ] = 888;
	geo_fence_info.latitude[ geo_fence_info.total_geo_fence ] = 37.393948;
	geo_fence_info.longitude [ geo_fence_info.total_geo_fence ] = 127.100736;
	geo_fence_info.range[ geo_fence_info.total_geo_fence ] = 100;
	geo_fence_info.total_geo_fence++;
	
	/*
	geo_fence_info.stop_num[ geo_fence_info.total_geo_fence ] = 888;
	geo_fence_info.latitude[ geo_fence_info.total_geo_fence ] = 0.0;
	geo_fence_info.longitude [ geo_fence_info.total_geo_fence ] = 0.0;
	geo_fence_info.range[ geo_fence_info.total_geo_fence ] = 0;
	geo_fence_info.total_geo_fence++;
	*/
	
	for (i = 0 ; i < geo_fence_info.total_geo_fence ; i++)
	{
		geo_fence_setup_t geo_fence_data = {0,};
		geo_fence_data.latitude = geo_fence_info.latitude[i];
		geo_fence_data.longitude = geo_fence_info.longitude[i];
		geo_fence_data.range = geo_fence_info.range[i];
		geo_fence_data.setup_fence_status = eFENCE_SETUP_ENTRY_EXIT;
		geo_fence_data.enable = eGEN_FENCE_ENABLE;
		set_geo_fence_setup_info(i, &geo_fence_data);
	}
	
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! geofence!!!!!!\r\n");
	
	save_geo_fence_setup_info();
	#endif

	return 0;
}
//static allocation_ftpserver_info_t ftp_server_info;
void alloc_ftpserver_info_init()
{
	memset(&ftp_server_info, 0x00, sizeof(allocation_ftpserver_info_t));

	sprintf(ftp_server_info.ftp_ip, "%s", ALLOCTION_FTP_IP);
	sprintf(ftp_server_info.ftp_port, "%s", ALLOCTION_FTP_PORT);
	sprintf(ftp_server_info.ftp_id, "%s", ALLOCTION_FTP_ID);
	sprintf(ftp_server_info.ftp_pw, "%s", ALLOCTION_FTP_PW);
	sprintf(ftp_server_info.filename,"%s/%s", DOWNLOAD_FILEPATH, DOWNLOAD_FILENAME);
}

int get_ftpserver_cmd_id(char* cmd_id)
{
	strncpy(cmd_id, ftp_server_info.cmd_id, 10);

	return 0;
}

int save_ftpserver_info(packet_frame_t result)
{
	char token[ ] = ",";
	//char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	
	int err = 0;
	
	char* buff_p = (char*)result.packet_content;
	char *dirc, *dname;
	char destfile[FILENAME_SIZE];

	FILE *fp;

	// ==============================================
	// 0 : pkt command 
	// ==============================================
	tr = strtok_r(buff_p, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}

	// ==============================================
	// 1 : phone num
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -2;
		goto CMD_PARSE_FAIL;
	}
	
	printf("   - save_ftpserver_info  phone num [%s]\r\n",tr);

	// ==============================================
	// 2 : cmd id
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -3;
		goto CMD_PARSE_FAIL;
	}
	if (1) // ..........
	{
		printf("   - save_ftpserver_info cmd id [%s]\r\n",tr);
		memset(&ftp_server_info.cmd_id, '\0', 10);
		strncpy((char*)ftp_server_info.cmd_id, tr, strlen(tr));
	}
	
	// ==============================================
	// 3 : ip
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -6;
		goto CMD_PARSE_FAIL;
	}
	
	//tp_server_info.ftp_ip = tr;

	memset(&ftp_server_info.ftp_ip, '\0', 20);
	strncpy((char*)ftp_server_info.ftp_ip, tr, strlen(tr));
	printf("   - ftp_server_info.ip [%s]\r\n", ftp_server_info.ftp_ip);

	// ==============================================
	// 4 : port
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -8;
		goto CMD_PARSE_FAIL;
	}

	memset(&ftp_server_info.ftp_port, '\0', 20);
	strncpy((char*)ftp_server_info.ftp_port, tr, strlen(tr));
	printf("   - ftp_server_info.ftp_port [%s]\r\n", ftp_server_info.ftp_port);

	// ==============================================
	// 5 : ID
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -10;
		goto CMD_PARSE_FAIL;
	}

	memset(&ftp_server_info.ftp_id, '\0', 20);
	strncpy((char*)ftp_server_info.ftp_id, tr, strlen(tr));
	printf("   - ftp_server_info.ftp_id [%s]\r\n", ftp_server_info.ftp_id);

	// ==============================================
	// 6 : pw
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -12;
		goto CMD_PARSE_FAIL;
	}

	memset(&ftp_server_info.ftp_pw, '\0', 20);
	strncpy((char*)ftp_server_info.ftp_pw, tr, strlen(tr));
	//printf("   - ftp_server_info.ftp_pw [%s]\r\n", ftp_server_info.ftp_pw);

	// ==============================================
	// 7 : file name
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -14;
		goto CMD_PARSE_FAIL;
	}
	
	memset(&ftp_server_info.filename, '\0', 32);
	strncpy((char*)ftp_server_info.filename, tr, strlen(tr));
	printf("   - ftp_server_info.filename [%s]\r\n", ftp_server_info.filename);
	
	int circulating_bus = 0;
	circulating_bus = get_circulating_bus();

	if (circulating_bus)
	{
		devel_webdm_send_log("[alloc Packet] circulating_bus !!!");
		printf("circulating_bus :  %d !!!\n", circulating_bus );
		g_tl500_state = 3;

		return 2;

	}

	sprintf(destfile, "%s%s", USER_DATA_DIR, ftp_server_info.filename);

	dirc = strdup(destfile);
	dname = dirname(dirc);

	printf("dirname=%s\n", dname);

	fp = fopen(destfile , "r");

	int size = 0;
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
    	size = ftell(fp);

		printf("size : %d !!!\n", size); 	
	}
	else
	{
		printf("fopen is null!!\n"); 	
	}	

	if (fp == NULL || size < 10) {
		int ret = 0;

		delete_ftpfolder(dname);

		devel_webdm_send_log("[alloc Packet] FTP DB FileName : %s start ftp client", (char*)ftp_server_info.filename);
		ret = Ftp_startClient((char *)ftp_server_info.ftp_ip, (char *)ftp_server_info.ftp_port, 
				(char *)ftp_server_info.ftp_id, (char *)ftp_server_info.ftp_pw, 
				(char *)ftp_server_info.filename, (char *)destfile);

		printf("Ftp_startClient ret :  %d !!!\n", ret);
		LOGI(LOG_TARGET, "Ftp_startClient ret :  %d !!!\n", ret);
		
		if(ret >= 0)
			set_alloc_rfid_download_DBfile((char*)destfile, (char*)ftp_server_info.filename);	
	}
	else
	{	

		fclose(fp);
		devel_webdm_send_log("[alloc Packet] FTP DB FileName : %s saved file !!!", ftp_server_info.filename);
		printf("%s: saved file !!! g_rfid_requestdb: %d, g_rfid_filename: %s\n" , destfile, g_rfid_requestdb, g_rfid_filename);

		if(g_rfid_requestdb > 0)
		{
			printf("g_rfid_requestdb !!!\n" );
			LOGI(LOG_TARGET, "g_rfid_requestdb update !!!\n");
			set_alloc_rfid_download_DBfile((char*)destfile, (char*)ftp_server_info.filename);
			g_rfid_requestdb = 0;
		}		

	}

	//set_alloc_rfid_download_DBfile(destfile, ftp_server_info.filename);	
	
	g_tl500_state = 3;

CMD_PARSE_FAIL:
	if (err < 0)
	{
		alloc_ftpserver_info_init();
	}
	
	return 2;
}
void print_ftpserver_info()
{
	printf("----------------- ftp_server_info info --------------------\r\n");
	printf("ftp_server_info.status [%d]\r\n",ftp_server_info.status);
	printf("ftp_server_info.cmd_id [%s]\r\n",ftp_server_info.cmd_id);
	printf("ftp_server_info.ip is [%s] \r\n",ftp_server_info.ftp_ip);
	printf("ftp_server_info.ftp_port is [%s] \r\n",ftp_server_info.ftp_port);
	printf("ftp_server_info.ftp_id is [%s] \r\n",ftp_server_info.ftp_id);
	//printf("ftp_server_info.ftp_pw is [%s] \r\n",ftp_server_info.ftp_pw);
	printf("ftp_server_info.ftp_port is [%s] \r\n",ftp_server_info.filename);
	
	printf("------------------------------------------------------\r\n");
}
int updatefirmware_daewoo(packet_frame_t result)
{
	char token[ ] = ",";
	//char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	
	int err = 0;
	dm_res res = DM_FAIL;
	UPDATE_VERS update_data;
	update_data.version = UPDATE_VCMD;
	
	char* buff_p = (char*)result.packet_content;
	char pftp_ip[20] = {0,};
	char pftp_port[20] = {0,};
	int  d_ftp_port = 0;
	char pftp_id[20] = {0,};
	char pftp_pw[20] = {0,};
	char pftp_filename[32] = {0,};	

	FILE *fp;

	// ==============================================
	// 0 : pkt command 
	// ==============================================
	tr = strtok_r(buff_p, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}

	// ==============================================
	// 2 : ip
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -6;
		goto CMD_PARSE_FAIL;
	}
	
	//tp_server_info.ftp_ip = tr;

	memset(&pftp_ip, '\0', 20);
	strncpy((char*)pftp_ip, tr, strlen(tr));
	printf("   - pftp_ip [%s]\r\n", pftp_ip);

	// ==============================================
	// 3 : port
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -8;
		goto CMD_PARSE_FAIL;
	}

	memset(&pftp_port, '\0', 20);
	strncpy((char*)pftp_port, tr, strlen(tr));
	printf("   - p_ftp_port [%s]\r\n", pftp_port);
	d_ftp_port = atoi(pftp_port);
	printf("   - d_ftp_port [%d]\r\n", d_ftp_port);

	// ==============================================
	// 4 : ID
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -10;
		goto CMD_PARSE_FAIL;
	}

	memset(&pftp_id, '\0', 20);
	strncpy((char*)pftp_id, tr, strlen(tr));
	printf("   - d_ftp_id[%s]\r\n", pftp_id);

	// ==============================================
	// 5 : pw
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -12;
		goto CMD_PARSE_FAIL;
	}

	memset(&pftp_pw, '\0', 20);
	strncpy((char*)pftp_pw, tr, strlen(tr));
	printf("   - d_ftp_pw [%s]\r\n", pftp_pw);

	// ==============================================
	// 6 : file name
	// ==============================================
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr == NULL) )
	{
		err = -14;
		goto CMD_PARSE_FAIL;
	}
	
	memset(&pftp_filename, '\0', 32);
	strncpy((char*)pftp_filename, tr, strlen(tr));
	printf("   - ftp_server_info.filename [%s]\r\n", pftp_filename);

	res =  dm_update_ftp_download(pftp_ip, d_ftp_port, pftp_id, pftp_pw, pftp_filename);


	if (res == DM_OK) {
		//_deinit_essential_functions();
		//terminate_app_callback();
		
		res = version_update(&update_data);
		if(res == success)
		{
			res = DM_OK;
		}
		else
		{
			printf("[dmlib] %s : update process failture\n", __func__);
			res = DM_FAIL;
		}
	}

	poweroff(__FUNCTION__, sizeof(__FUNCTION__));


CMD_PARSE_FAIL:
	
	return 1;
}
////////////////////////////////////////////////////////////////////////////////////////
// pkt util
////////////////////////////////////////////////////////////////////////////////////////
unsigned char convert_angle(float azimuth)
{
	int bearing;

	bearing = RoundOff(azimuth, 0);

	if(bearing == 0) {
		return eNORTH_DIR;
	} else if((bearing > 0) && (bearing < 90)) {
		return eNORTH_EAST_DIR;
	} else if(bearing == 90) {
		return eEAST_DIR;
	} else if((bearing > 90) && (bearing < 180)) {
		return eSOUTH_EAST_DIR;
	} else if(bearing == 180) {
		return eSOUTH_DIR;
	} else if((bearing > 180) && (bearing < 270)) {
		return eSOUTH_WEST_DIR;
	} else if(bearing == 270) {
		return eWEST_DIR;
	} else if((bearing > 270) && (bearing <= 360)) {
		return eNORTH_WEST_DIR;
	}

	return 0xff;
}


char _alloc_get_binary_packet_id(packet_frame_t result)
{
	char token[ ] = ",";
	char *temp_bp = NULL;
	char *tr = NULL;
	
	char err = 0;

	char* buff_p = result.packet_content;
	
	//printf("start [%s]\r\n",buff_p);
	
	// 0 : pkt command 
	tr = strtok_r(buff_p, token, &temp_bp);
	
	if ( (tr == NULL) )
	{
		err = 0;
		//printf("fail??\r\n");
		goto CMD_PARSE_FAIL;
	}
	else
	{
		//printf("_get_binary_packet arg 0 [%s]\r\n", tr);
		
	}

	return tr[0];
CMD_PARSE_FAIL:
	return err;
}

int _alloc_get_packet_frame(const char* buff, int buff_len, packet_frame_t* result)
{
	static unsigned char _saved_buff[ PKT_FRAME_QUEUE_BUFF_SIZE ] = {0,};
	
	static int queue_top_idx = 1;
	static int queue_bottom_idx = 0;
	
	int found_prefix = -1;
	int found_suffix = -1;
	
	int i = 0;
	int j = 0;
	//printf("_get_packet_frame ++++++++++++++++++ \r\n");
	
	//printf("cueue status start [%d] / [%d]\r\n", queue_top_idx, queue_bottom_idx);
	
	// 들어온 버퍼만큼 모두 큐에 저장.
	for ( i = 0 ; i < buff_len ; i++ )
	{
		//printf("insert queue -1 [%d/%d] \r\n",queue_top_idx, queue_bottom_idx);
		
		_saved_buff [ queue_top_idx ] = buff[i];
		
		// printf("cueue status -- 1 [%d] / [%d]\r\n", queue_top_idx, queue_bottom_idx);
		
		// top idx increase
		//queue_top_idx = ((queue_top_idx++) % PKT_FRAME_QUEUE_BUFF_SIZE );
		
		queue_top_idx++;
		queue_top_idx = (queue_top_idx % PKT_FRAME_QUEUE_BUFF_SIZE);
		
		// printf("cueue status -- 2 [%d] / [%d]\r\n", queue_top_idx, queue_bottom_idx);
		//printf("insert queue -2 [%d/%d] \r\n",queue_top_idx, queue_bottom_idx);
		
		// bottom idx increase
		if ( queue_top_idx == queue_bottom_idx )
		{
			queue_bottom_idx++;
			queue_bottom_idx = (queue_bottom_idx % PKT_FRAME_QUEUE_BUFF_SIZE);
		}
	}
	
	//printf("cueue status -1 [%d] / [%d] \r\n", queue_top_idx, queue_bottom_idx);
	
	// ----------------------------------------------------
	// find PACKET_START_CHAR

	
	for ( i = 0 ; i < PKT_FRAME_QUEUE_BUFF_SIZE; i++)
	{
		int check_targ_idx = (queue_bottom_idx + i) % PKT_FRAME_QUEUE_BUFF_SIZE ;
		char chk_char = 0;

		chk_char = _saved_buff[ check_targ_idx ];
		
		//printf("chk star char [%c]\r\n", chk_char);
		
		if ( chk_char == PACKET_START_CHAR )
		{
			//printf("found start char ... [%d]\r\n",check_targ_idx);
			found_prefix = check_targ_idx;
			break;
		}
		
		if ( check_targ_idx == queue_top_idx )
		{
			// printf("found not char ... \r\n");
			break;
		}
	}
	
	if ( found_prefix == -1 )
	{
		queue_bottom_idx = (queue_top_idx - 1);
		//printf("cannot found prefix !!!\r\n");
		return -1;
	}
	
	// ----------------------------------------------------
	// find PACKET_END_CHAR
	
	for ( i = 0 ; i < PKT_FRAME_QUEUE_BUFF_SIZE; i++)
	{
		int check_targ_idx = (found_prefix + i) % PKT_FRAME_QUEUE_BUFF_SIZE;
		char chk_char = 0;
		
		chk_char = _saved_buff[ check_targ_idx ];
		
		if ( chk_char == PACKET_END_CHAR )
		{
			// printf("found end char ... [%d]\r\n",i);
			found_suffix = check_targ_idx;
			break;
		}
		
		if ( check_targ_idx == queue_top_idx )
		{
			// printf("found not char ... \r\n");
			//printf("cannot found suffix !!!\r\n");
			break;
		}
	}
	
	if ( found_suffix == -1 )
	{
		//queue_bottom_idx = queue_top_idx;
		return -1;
	}
	
	
	for ( i = 0 ; i < PKT_FRAME_QUEUE_BUFF_SIZE; i++)
	{
		int cur_idx = (found_prefix + i) % PKT_FRAME_QUEUE_BUFF_SIZE;
		
		if ( ( _saved_buff[cur_idx] != PACKET_START_CHAR ) && ( _saved_buff[cur_idx] != PACKET_END_CHAR ) )
		{
			//printf("_saved_buff[cur_idx] is [%c]\r\n",_saved_buff[cur_idx]);
			result->packet_content[j++] = _saved_buff[cur_idx];
		}
		
		if ( cur_idx == found_suffix )
			break;
	}
	
	result->size = j;

	// jhcho 뒤에 {3, 이 더 붙는 경우 처리 [[
	if((result->size + 2) <  buff_len)
	{
		int index = result->size + 2;
		b_ex_buff = 1;
		for (i = 0; i < (buff_len - index); i++)
		{
			_ex_buff[i] = _saved_buff[index + i + 1];
		}
		print_yellow("ex_buff : %s \n", _ex_buff);
	}
	// jhcho 뒤에 {3, 이 더 붙는 경우 처리 ]]
	print_yellow("size : %d, buff_len : %d \n", result->size, buff_len);
	
	queue_bottom_idx = found_suffix % PKT_FRAME_QUEUE_BUFF_SIZE;
	
	// found_suffix++;
	
	// queue_bottom_idx = (found_suffix++ % PKT_FRAME_QUEUE_BUFF_SIZE);
	
	//printf("cueue status end [%d] / [%d]\r\n", queue_top_idx, queue_bottom_idx);
	//printf("_get_packet_frame ++++++++++++++++++ \r\n");

	//buffer clearing jwrho ++
	memset(_saved_buff, 0x00, sizeof(_saved_buff));
	queue_top_idx = 1;
	queue_bottom_idx = 0;	
	printf("_alloc_get_packet_frame buffer clearing\n");
	//jwrho--
	return 0;
}

//jwrho ++
int get_geofence_status()
{
	return geo_fence_info.status;
}
//jwrho --

void delete_ftpfolder(char *dname)
{
	char strmkdir[36]; 
	char strRm[36];

	sprintf(strmkdir, "mkdir -p %s",dname);
	system(strmkdir);

	sprintf(strRm, "rm %s/*.*",dname);
	system(strRm);
	printf("delete_ftpfolder\n" );
}
