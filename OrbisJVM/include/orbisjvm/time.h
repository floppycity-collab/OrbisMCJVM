/**
 * @file time.h
 * @brief OrbisJVM Time API
 *
 * Provides time functions compatible with Orbis OS RTC API.
 */

#ifndef ORBISJVM_TIME_H
#define ORBISJVM_TIME_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clock type
 */
typedef enum {
    ORBISJVM_CLOCK_REALTIME     = 0,  ///< Wall clock time
    ORBISJVM_CLOCK_MONOTONIC    = 1,  ///< Monotonic clock (never goes backwards)
    ORBISJVM_CLOCK_MONOTONIC_RAW = 2, ///< Raw hardware clock
    ORBISJVM_CLOCK_PROCESS_CPUTIME = 3, ///< Process CPU time
    ORBISJVM_CLOCK_THREAD_CPUTIME  = 4, ///< Thread CPU time
} orbisjvm_clock_t;

/**
 * @brief Time specification (nanosecond resolution)
 */
typedef struct {
    int64_t tv_sec;   ///< Seconds
    int64_t tv_nsec;  ///< Nanoseconds (0-999999999)
} orbisjvm_timespec_t;

/**
 * @brief Time value (microsecond resolution)
 */
typedef struct {
    int64_t tv_sec;   ///< Seconds
    int64_t tv_usec;  ///< Microseconds (0-999999)
} orbisjvm_timeval_t;

/**
 * @brief Timezone information
 */
typedef struct {
    int tz_minuteswest;  ///< Minutes west of Greenwich
    int tz_dsttime;      ///< DST correction type
} orbisjvm_timezone_t;

/**
 * @brief Get current time
 *
 * @param clock Clock type
 * @param[out] ts Output time specification
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_clock_gettime(orbisjvm_clock_t clock, orbisjvm_timespec_t* ts);

/**
 * @brief Get clock resolution
 *
 * @param clock Clock type
 * @param[out] res Output resolution
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_clock_getres(orbisjvm_clock_t clock, orbisjvm_timespec_t* res);

/**
 * @brief Get time of day (wall clock)
 *
 * @param[out] tv Output time value
 * @param[out] tz Timezone (or NULL)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_gettimeofday(orbisjvm_timeval_t* tv, orbisjvm_timezone_t* tz);

/**
 * @brief Get monotonic time in nanoseconds
 *
 * @return Monotonic time in nanoseconds
 */
uint64_t orbisjvm_get_monotonic_ns(void);

/**
 * @brief Get monotonic time in microseconds
 *
 * @return Monotonic time in microseconds
 */
uint64_t orbisjvm_get_monotonic_us(void);

/**
 * @brief Get monotonic time in milliseconds
 *
 * @return Monotonic time in milliseconds
 */
uint64_t orbisjvm_get_monotonic_ms(void);

/**
 * @brief Get wall clock time in nanoseconds
 *
 * @return Wall clock time in nanoseconds since epoch
 */
uint64_t orbisjvm_get_walltime_ns(void);

/**
 * @brief Get wall clock time in microseconds
 *
 * @return Wall clock time in microseconds since epoch
 */
uint64_t orbisjvm_get_walltime_us(void);

/**
 * @brief Get wall clock time in milliseconds
 *
 * @return Wall clock time in milliseconds since epoch
 */
uint64_t orbisjvm_get_walltime_ms(void);

/**
 * @brief Convert timespec to nanoseconds
 *
 * @param ts Time specification
 * @return Nanoseconds
 */
static inline uint64_t orbisjvm_timespec_to_ns(const orbisjvm_timespec_t* ts) {
    return (uint64_t)ts->tv_sec * 1000000000ULL + (uint64_t)ts->tv_nsec;
}

/**
 * @brief Convert nanoseconds to timespec
 *
 * @param ns Nanoseconds
 * @param[out] ts Output time specification
 */
static inline void orbisjvm_ns_to_timespec(uint64_t ns, orbisjvm_timespec_t* ts) {
    ts->tv_sec = (int64_t)(ns / 1000000000ULL);
    ts->tv_nsec = (int64_t)(ns % 1000000000ULL);
}

/**
 * @brief Convert timeval to microseconds
 *
 * @param tv Time value
 * @return Microseconds
 */
static inline uint64_t orbisjvm_timeval_to_us(const orbisjvm_timeval_t* tv) {
    return (uint64_t)tv->tv_sec * 1000000ULL + (uint64_t)tv->tv_usec;
}

/**
 * @brief Convert microseconds to timeval
 *
 * @param us Microseconds
 * @param[out] tv Output time value
 */
static inline void orbisjvm_us_to_timeval(uint64_t us, orbisjvm_timeval_t* tv) {
    tv->tv_sec = (int64_t)(us / 1000000ULL);
    tv->tv_usec = (int64_t)(us % 1000000ULL);
}

/**
 * @brief Sleep for specified nanoseconds
 *
 * @param ns Nanoseconds to sleep
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_nanosleep(const orbisjvm_timespec_t* req, orbisjvm_timespec_t* rem);

/**
 * @brief Sleep for specified microseconds
 *
 * @param us Microseconds to sleep
 */
void orbisjvm_usleep(uint64_t us);

/**
 * @brief Sleep for specified milliseconds
 *
 * @param ms Milliseconds to sleep
 */
void orbisjvm_msleep(uint64_t ms);

/**
 * @brief Sleep for specified seconds
 *
 * @param seconds Seconds to sleep
 */
void orbisjvm_sleep(uint64_t seconds);

/**
 * @brief High-resolution timer handle
 */
typedef struct orbisjvm_timer* orbisjvm_timer_t;

/**
 * @brief Timer callback
 *
 * @param timer Timer handle
 * @param user_data User data
 */
typedef void (*orbisjvm_timer_cb_t)(orbisjvm_timer_t timer, void* user_data);

/**
 * @brief Timer flags
 */
typedef enum {
    ORBISJVM_TIMER_NONE      = 0,
    ORBISJVM_TIMER_ONESHOT   = (1 << 0),  ///< Fire once
    ORBISJVM_TIMER_PERIODIC  = (1 << 1),  ///< Fire periodically
    ORBISJVM_TIMER_HIGH_RES  = (1 << 2),  ///< High resolution (if supported)
} orbisjvm_timer_flags_t;

/**
 * @brief Create a timer
 *
 * @param[out] timer Output timer handle
 * @param clock Clock to use
 * @param flags Timer flags
 * @param callback Callback function
 * @param user_data User data passed to callback
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_timer_create(orbisjvm_timer_t* timer,
                          orbisjvm_clock_t clock,
                          orbisjvm_timer_flags_t flags,
                          orbisjvm_timer_cb_t callback,
                          void* user_data);

/**
 * @brief Start a timer
 *
 * @param timer Timer handle
 * @param initial_delay Initial delay in nanoseconds
 * @param interval Interval in nanoseconds (0 for one-shot)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_timer_start(orbisjvm_timer_t timer, uint64_t initial_delay, uint64_t interval);

/**
 * @brief Stop a timer
 *
 * @param timer Timer handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_timer_stop(orbisjvm_timer_t timer);

/**
 * @brief Destroy a timer
 *
 * @param timer Timer handle
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_timer_destroy(orbisjvm_timer_t timer);

/**
 * @brief Get timer overrun count
 *
 * @param timer Timer handle
 * @return Overrun count
 */
uint64_t orbisjvm_timer_get_overrun(orbisjvm_timer_t timer);

/**
 * @brief Convert RTC time table to time_t
 *
 * @param tt RTC time table
 * @return time_t value
 */
time_t orbisjvm_rtc_to_time_t(const struct orbisjvm_rtc_timetable* tt);

/**
 * @brief Convert time_t to RTC time table
 *
 * @param t time_t value
 * @param[out] tt Output RTC time table
 */
void orbisjvm_time_t_to_rtc(time_t t, struct orbisjvm_rtc_timetable* tt);

/**
 * @brief RTC time table (from Orbis RTC API)
 */
typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t microsecond;
} orbisjvm_rtc_timetable_t;

/**
 * @brief Get current RTC time
 *
 * @param[out] tt Output time table
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rtc_get_current(orbisjvm_rtc_timetable_t* tt);

/**
 * @brief Set RTC time (requires privileges)
 *
 * @param tt Time table
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_rtc_set_current(const orbisjvm_rtc_timetable_t* tt);

/**
 * @brief Format time as ISO 8601 string
 *
 * @param ts Time specification
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_format_iso8601(const orbisjvm_timespec_t* ts, char* buffer, size_t buflen);

/**
 * @brief Format time as RFC 3339 string
 *
 * @param ts Time specification
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_format_rfc3339(const orbisjvm_timespec_t* ts, char* buffer, size_t buflen);

/**
 * @brief Parse ISO 8601 time string
 *
 * @param str ISO 8601 string
 * @param[out] ts Output time specification
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_parse_iso8601(const char* str, orbisjvm_timespec_t* ts);

/**
 * @brief Time statistics (for diagnostics)
 */
typedef struct {
    uint64_t monotonic_freq_hz;  ///< Monotonic clock frequency
    int monotonic_is_steady;     ///< Whether monotonic clock is steady
    uint64_t timer_resolution_ns;///< Timer resolution
} orbisjvm_time_stats_t;

/**
 * @brief Get time statistics
 *
 * @param[out] stats Output statistics
 */
void orbisjvm_get_time_stats(orbisjvm_time_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_TIME_H