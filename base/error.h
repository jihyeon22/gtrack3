#ifndef __BASE_ERROR_H__
#define __BASE_ERROR_H__

#define LOG_ERR_MAX_COUNT		50
#define LOG_CRITICAL_MAX_COUNT	10
#define MAX_RETRY_EXIT			3
#define MAX_RETRY_REBOOT		3
//jwrho ++
#pragma pack(push, 1)

typedef struct {
	char date[25];
	char contents[530];
}__attribute__((packed))MSG_LOG;

#pragma pack(pop)
//jwrho --

typedef enum errorType errorType_t;
enum errorType
{
	eERROR_NONE,
	eERROR_LOG,
	eERROR_EXIT,
	eERROR_REBOOT,
	eERROR_FINAL
};

typedef struct errorData errorData_t;
struct errorData
{
	int no_exit;
	int no_reboot;
};

typedef struct errorDataFile errorDataFile_t;
struct errorDataFile
{
	char chr_no_exit;
	char chr_no_reboot;
};

void error_critical(const int level, const char *format, ...);
void error_make_no_mon(void);
void error_rm_no_mon(void);

extern int gError_state;

#define WORKING_PROPERLY_SECS 10*60

#endif

