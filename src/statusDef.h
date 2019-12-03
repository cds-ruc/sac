/*
 * Configure file of SSD-SMR cache system.
 * All the switch is turn-off by default.
 */


/** Log reporting level **/

#define LOG_ALLOW // Log allowed EXCLUSIVELY for 1. Print in Emulator.
#undef LOG_SINGLE_REQ // LEGACY: Print detail time information of each single request.
#undef LOG_IO_LAT // report each io latency. 


/** Emulator Related **/
#undef EMULATION_AIO
#define EMU_NO_DISK_IO

/* Future Features */
