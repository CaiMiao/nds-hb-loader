#ifndef __CONFIGURATOR_H__
#define __CONFIGURATOR_H__

// #include "scsfw_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENTRY_SIZE 256

typedef struct ENTRY {
	char path[ENTRY_SIZE];
} ENTRY;

typedef struct HBLDR_CONFIGS {
	ENTRY hk_a;
	ENTRY hk_b;
	ENTRY hk_x;
	ENTRY hk_y;
	ENTRY hk_l;
	ENTRY hk_r;
	ENTRY hk_none;
} HBLDR_CONFIGS;

bool readConfigsFromFile(struct HBLDR_CONFIGS* configs);
bool dumpConfigsToFile(struct HBLDR_CONFIGS* configs);
void configMenu(struct HBLDR_CONFIGS* configs);

#ifdef __cplusplus
}
#endif

#endif //__SCSFW_CONFIG_H__