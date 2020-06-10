#pragma once
#include <board/board_system.h>

#ifdef TEST_CODE_ENABLE
	#define LOTTE_DEVICE_MILEAGE_FILE	"./moram_mileage.dat"
#else
	//jwrho persistant data path modify++
	//#define LOTTE_DEVICE_MILEAGE_FILE	"/data/mds/data/moram_mileage.dat"
	#define LOTTE_DEVICE_MILEAGE_FILE	CONCAT_STR(USER_DATA_DIR, "/moram_mileage.dat")
	//jwrho persistant data path modify--
#endif

#pragma pack(push, 1)

typedef struct {
	int mileage;
}__attribute__((packed))lotte_mileage_t;

int load_mileage_file();
int save_mileage_file(int mileage);

#pragma pack(pop)
