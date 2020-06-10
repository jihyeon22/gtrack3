#pragma once

#pragma pack(push, 1)

typedef struct {
	int mileage;
}__attribute__((packed))lotte_mileage_t;

int load_mileage_file();
int save_mileage_file(int mileage);

#pragma pack(pop)
