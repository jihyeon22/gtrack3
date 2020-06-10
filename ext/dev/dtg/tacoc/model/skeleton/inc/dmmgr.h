#ifndef __BASE_DMMGR_H__
#define __BASE_DMMGR_H__

typedef enum dmEventType dmEventType_t;
enum dmEventType
{
	eEVENT_PWR_ON = 0,
	eEVENT_PWR_OFF,
	eEVENT_KEY_ON,
	eEVENT_KEY_OFF,
	eEVENT_REPORT,
	eEVENT_STATUS,
	eEVENT_LOG,
	eEVENT_WARNING,
	eEVENT_BREAKDOWN,
	eEVENT_UPDATE,
	eEVENT_MAX
};

int // dmmgr_init(char *ini, char *pkg);
int dmmgr_send(dmEventType_t type, const char *log, int evo_errno);
void dmmgr_deinit(void);
void dmmgr_send_incomplete_event(void);

#endif /* __BASE_DMMGR_H__ */
