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

#define TIME_TO_SEND_ALIVE	(60*60*12)

void dmmgr_init(void);
int dmmgr_send(dmEventType_t type, const char *log, int evo_errno);
void dmmgr_deinit(void);
void dmmgr_send_incomplete_event(void);
int dmmgr_sended_key_on(void);
void dmmgr_alive_send(void);

#endif /* __BASE_DMMGR_H__ */
