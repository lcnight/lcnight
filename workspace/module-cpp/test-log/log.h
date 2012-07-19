/**
 *============================================================
 *  @file      log.h
 *  @brief     用于记录日志，一共分9种日志等级。日志文件可以自动轮转。注意：如果使用自动轮转，\n
 *             必须保证每天写的日志文件个数不能超过log_init时设定的最大文件个数，否则日志会写乱掉。
 *             必须先调用log_init来初始化日志功能。注意，每条日志不能超过8000字节。\n
 *             如果编译程序时定义宏LOG_USE_SYSLOG，则会利用syslog来记录日志，使用的facility是LOG_USER。
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_LOG_H_
#define LIBTAOMEE_LOG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef log_lvl_t
 * @brief   typedef of log_lvl
 */

/**
 * @enum  log_lvl
 * @brief 日志等级
 */
typedef enum log_lvl {
	log_lvl_emerg,
	log_lvl_alert,
	log_lvl_crit,
	log_lvl_error,
	log_lvl_warning,
	log_lvl_notice,
	log_lvl_info,
	log_lvl_debug,
#ifndef LOG_USE_SYSLOG
	log_lvl_trace,
#else
	log_lvl_trace = log_lvl_debug,
#endif
	log_lvl_max
} log_lvl_t;

/**
 * @typedef log_dest_t
 * @brief   typedef of log_dest
 */

/**
 * @enum  log_dest
 * @brief 日志输出方式
 */
typedef enum log_dest {
	log_dest_terminal	= 1,
	log_dest_file		= 2,
	log_dest_both		= 3
} log_dest_t;

/**
  * @brief 初始化日志记录功能。如果使用自动轮转，必须保证每天写的日志文件个数不能超过最大文件个数（maxfiles），
  *        否则日志会写乱掉。
  *
  * @param const char* dir,  日志保存目录。如果填0，则在屏幕中输出日志。
  * @param int lvl,  日志输出等级。如果设置为log_lvl_notice，则log_lvl_notice以上等级的日志都不输出。
  * @param uint32_t size,  每个日志文件的大小限制（byte），超过这个大小则自动创建一个新的日志文件。
  * @param int maxfiles,  每种等级的日志的最大文件个数，用于控制日志轮转。个数越少，日志轮转时效率越高。
  *                       如果填0，则表示不使用日志轮转功能。
  * @param const char* pre_name,  日志文件名前缀。
  *
  * @see set_log_dest
  *
  * @return int, 成功则返回0，失败则退出程序。
  */
int  log_init(const char* dir, log_lvl_t lvl, uint32_t size, int maxfiles, const char* pre_name);

/**
 * @brief 调用log_init初始化日志功能后，可以调用该函数动态调整日志的输出方式。如果不调用set_log_dest的话，
 *        则输出方式为log_init时确定的方式。注意：必须在log_init时指定了日志保存目录，才可以调用该函数。
 *
 * @param dest 日志输出方式
 *
 * @see log_init
 */
void set_log_dest(log_dest_t dest);

/**
 * @brief 这个写日志的库默认不支持多线程！多线程程序写日志的话，需要调用先一下这个函数，否则会有问题。
 *
 */
void enable_multi_thread();

void write_log(int lvl, const char* fmt, ...);
void write_syslog(int lvl, const char* fmt, ...);
void boot_log(int ok, int dummy, const char* fmt, ...);

#ifndef LOG_USE_SYSLOG
#define DETAIL(level, fmt, args...) \
		write_log(level, "[%s][%d]%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define DETAIL(level, fmt, args...) \
		write_syslog(level, "[%s][%d]%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#endif

#ifndef LOG_USE_SYSLOG
#define SIMPLY(level, fmt, args...) \
		write_log(level, fmt "\n", ##args)
#else
#define SIMPLY(level, fmt, args...) \
		write_syslog(level, fmt "\n", ##args)
#endif

/**
 * @def EMERG_LOG
 * @brief 输出log_lvl_emerg等级日志。如果定义宏DISABLE_EMERG_LOG，则可以在编译期把EMERG_LOG去除。\n
 *        用法示例：EMERG_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_EMERG_LOG
#define EMERG_LOG(fmt, args...) \
		DETAIL(log_lvl_emerg, fmt, ##args)
#else
#define EMERG_LOG(fmt, args...)
#endif

/**
 * @def ALERT_LOG
 * @brief 输出log_lvl_alert等级日志。如果定义宏DISABLE_ALERT_LOG，则可以在编译期把ALERT_LOG去除。\n
 *        用法示例：ALERT_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_ALERT_LOG
#define ALERT_LOG(fmt, args...) \
		DETAIL(log_lvl_alert, fmt, ##args)
#else
#define ALERT_LOG(fmt, args...)
#endif

/**
 * @def CRIT_LOG
 * @brief 输出log_lvl_crit等级日志。如果定义宏DISABLE_CRIT_LOG，则可以在编译期把CRIT_LOG去除。\n
 *        用法示例：CRIT_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_CRIT_LOG
#define CRIT_LOG(fmt, args...) \
		DETAIL(log_lvl_crit, fmt, ##args)
#else
#define CRIT_LOG(fmt, args...)
#endif

/**
 * @def ERROR_LOG
 * @brief 输出log_lvl_error等级日志。如果定义宏DISABLE_ERROR_LOG，则可以在编译期把ERROR_LOG去除。\n
 *        用法示例：ERROR_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_ERROR_LOG
#define ERROR_LOG(fmt, args...) \
		DETAIL(log_lvl_error, fmt, ##args)
#else
#define ERROR_LOG(fmt, args...)
#endif

/**
 * @def WARN_LOG
 * @brief 输出log_lvl_warning等级日志。如果定义宏DISABLE_WARN_LOG，则可以在编译期把WARN_LOG去除。\n
 *        用法示例：WARN_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_WARN_LOG
#define WARN_LOG(fmt, args...) \
		SIMPLY(log_lvl_warning, fmt, ##args)
#else
#define WARN_LOG(fmt, args...)
#endif

/**
 * @def NOTI_LOG
 * @brief 输出log_lvl_notice等级日志。如果定义宏DISABLE_NOTI_LOG，则可以在编译期把NOTI_LOG去除。\n
 *        用法示例：NOTI_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_NOTI_LOG
#define NOTI_LOG(fmt, args...) \
		SIMPLY(log_lvl_notice, fmt, ##args)
#else
#define NOTI_LOG(fmt, args...)
#endif

/**
 * @def INFO_LOG
 * @brief 输出log_lvl_info等级日志。如果定义宏DISABLE_INFO_LOG，则可以在编译期把INFO_LOG去除。\n
 *        用法示例：INFO_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_INFO_LOG
#define INFO_LOG(fmt, args...) \
		SIMPLY(log_lvl_info, fmt, ##args)
#else
#define INFO_LOG(fmt, args...)
#endif

/**
 * @def DEBUG_LOG
 * @brief 输出log_lvl_debug等级日志。如果定义宏DISABLE_DEBUG_LOG，则可以在编译期把DEBUG_LOG去除。\n
 *        用法示例：DEBUG_LOG("dlopen error, %s", error);
 */
#ifndef DISABLE_DEBUG_LOG
#define DEBUG_LOG(fmt, args...) \
		SIMPLY(log_lvl_debug, fmt, ##args)
#else
#define DEBUG_LOG(fmt, args...)
#endif

/**
 * @def TRACE_LOG
 * @brief 输出log_lvl_trace等级日志。如果不定义宏ENABLE_TRACE_LOG，则可以在编译期把TRACE_LOG去除。\n
 *        如果编译时定义了宏LOG_USE_SYSLOG，则TRACE_LOG日志会写到DEBUG_LOG日志文件里。\n
 *        用法示例：TRACE_LOG("dlopen error, %s", error);
 */
#ifdef ENABLE_TRACE_LOG
#define TRACE_LOG(fmt,args...) \
		DETAIL(log_lvl_trace, fmt, ##args)
#else
#define TRACE_LOG(fmt,args...)
#endif

/**
 * @def BOOT_LOG
 * @brief 输出程序启动日志到屏幕。如果OK非0，则退出程序；如果OK为0，则返回上一级函数。\n
 *        用法示例：BOOT_LOG(-1, "dlopen error, %s", error);
 */
#define BOOT_LOG(OK, fmt, args...) \
		do { \
			boot_log(OK, 0, fmt, ##args); \
			return OK; \
		} while (0)

/**
 * @def BOOT_LOG2
 * @brief 输出程序启动日志到屏幕。如果OK非0，则退出程序；如果OK为0，则返回上一级函数。n是空格填充个数。\n
 *        用法示例：BOOT_LOG2(0, 8, "dlopen ok");
 */
#define BOOT_LOG2(OK, n, fmt, args...) \
		do { \
			boot_log(OK, n, fmt , ##args); \
			return OK; \
		} while (0)

/**
 * @def ERROR_RETURN
 * @brief 输出log_lvl_error等级的日志，并且返回Y到上一级函数。\n
 *        用法示例：ERROR_RETURN(("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno)), -1);
 */
#define ERROR_RETURN(X, Y) \
		do { \
			ERROR_LOG X; \
			return Y; \
		} while (0)

/**
 * @def ERROR_RETURN_VOID
 * @brief 输出log_lvl_error等级的日志，并且返回上一级函数。\n
 *        用法示例：ERROR_RETURN("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno));
 */
#define ERROR_RETURN_VOID(fmt, args...) \
		do { \
			ERROR_LOG(fmt, ##args); \
			return; \
		} while (0)

/**
 * @def WARN_RETURN
 * @brief 输出log_lvl_warning等级的日志，并且返回ret_到上一级函数。\n
 *        用法示例：WARN_RETURN(("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno)), -1);
 */
#define WARN_RETURN(msg_, ret_) \
		do { \
			WARN_LOG msg_; \
			return (ret_); \
		} while (0)

/**
 * @def WARN_RETURN_VOID
 * @brief 输出log_lvl_warning等级的日志，并且返回上一级函数。\n
 *        用法示例：WARN_RETURN_VOID("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno));
 */
#define WARN_RETURN_VOID(fmt, args...) \
		do { \
			WARN_LOG(fmt, ##args); \
			return; \
		} while (0)

/**
 * @def DEBUG_RETURN
 * @brief 输出log_lvl_debug等级的日志，并且返回ret_到上一级函数。\n
 *        用法示例：DEBUG_RETURN(("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno)), -1);
 */
#define DEBUG_RETURN(msg_, ret_) \
		do { \
			DEBUG_LOG msg_; \
			return (ret_); \
		} while (0)

/**
 * @def DEBUG_RETURN_VOID
 * @brief 输出log_lvl_debug等级的日志，并且返回上一级函数。\n
 *        用法示例：DEBUG_RETURN_VOID("Failed to Create `mcast_fd`: err=%d %s", errno, strerror(errno));
 */
#define DEBUG_RETURN_VOID(fmt, args...) \
		do { \
			DEBUG_LOG(fmt, ##args); \
			return; \
		} while (0)

#ifdef __cplusplus
}
#endif

#endif // LIBTAOMEE_LOG_H_

