#define CONFIG_ATP_H_RANDOM_BOOT
//#define CONFIG_ATP_H_RANDOM_DB
#define CONFIG_ATP_H_FLEXIBLE_DB
#define CONFIG_ATP_H_MEMORYLESS_STATISTICS
#define CONFIG_ATP_H_DISABLE_SCL
//#define CONFIG_ATP_H_LQI_FILTERING
//#define CONFIG_ATP_H_RSSI_FILTERING

#define DEBUG_ATP_H
#ifdef DEBUG_ATP_H
	#define DEBUG_ATP_H_ENABLE
	#define DEBUG_ATP_H_ATP_SERVICE
	#define DEBUG_ATP_H_ATP_SERVICE_DISABLE
	#define DEBUG_ATP_H_EVENTS_CALLBACK
#endif
#define DEBUG_ATP_H_STATS
	#ifdef DEBUG_ATP_H_STATS
	//#define DEBUG_ATP_H_STATS_SHAWN
	#define DEBUG_ATP_H_STATS_ISENSE
#endif