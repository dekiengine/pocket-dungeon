#pragma once

// DUNGEON_HOT marks the code that runs for every pixel or column. On the Pico
// it runs from RAM: from flash, its instruction fetches compete with the
// textures for the 16 KB flash cache, and each miss stalls for a flash read.
#if defined(DEKI_PICO)
#define DUNGEON_HOT __attribute__((section(".time_critical.dungeon")))
#else
#define DUNGEON_HOT
#endif
