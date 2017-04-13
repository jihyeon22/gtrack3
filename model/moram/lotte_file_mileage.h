#pragma once

#ifdef TEST_CODE_ENABLE
	#define LOTTE_DEVICE_MILEAGE_FILE	"./moram_mileage.dat"
#else
	#define LOTTE_DEVICE_MILEAGE_FILE	"/data/mds/data/moram_mileage.dat"
#endif

#pragma pack(push, 1)

typedef struct {
	int mileage;
}__attribute__((packed))lotte_mileage_t;

int load_mileage_file();
int save_mileage_file(int mileage);

#pragma pack(pop)
