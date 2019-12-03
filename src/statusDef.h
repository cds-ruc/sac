/*
 * Configure file of SSD-SMR cache system.
 * All the switch is turn-off by default.
 */

/** ENV **/
#define PROJ_ROOT 
#define PATH_LOG  
#define PATH_SRC 
#define PATH_EMU


/** Log reporting level **/

#undef LOG_ALLOW // Log allowed EXCLUSIVELY for 1. Print in Emulator.
#undef LOG_SINGLE_REQ // LEGACY: Print detail time information of each single request.
#undef LOG_IO_LAT // report each io latency. 


/** Emulator Related **/
#undef EMULATION_AIO
#define EMU_NO_DISK_IO

/* Future Features */
