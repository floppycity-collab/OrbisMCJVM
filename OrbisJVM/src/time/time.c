/**
 * @file time.c
 * @brief OrbisJVM Time Implementation
 *
 * Implements time functions using Orbis OS RTC API and standard POSIX clocks.
 */

#include <orbisjvm/time.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/threading.h>

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Timer structure */
struct orbisjvm_timer {
    timer_t timer_id;
    orbisjvm_clock_t clock;
    orbisjvm_timer_flags_t flags;
    orbisjvm_timer_cb_t callback;
    void* user_data;
    int active;
};

/* Timer callback wrapper */
static void timer_callback_wrapper(union sigval sv) {
    struct orbisjvm_timer* timer = (struct orbisjvm_timer*)sv.sival_ptr;
    if (timer && timer->callback && timer->active) {
        timer->callback(timer, timer->user_data);
    }
}

/**
 * @brief Initialize time subsystem
 */
int orbisjvm_time_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Initializing time subsystem");
    return 0;
}

/**
 * @brief Shutdown time subsystem
 */
void orbisjvm_time_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_GENERAL, "Shutting down time subsystem");
}

/* Clock functions */

int orbisjvm_clock_gettime(orbisjvm_clock_t clock, orbisjvm_timespec_t* ts) {
    if (!ts) return -1;

    clockid_t clk;
    switch (clock) {
        case ORBISJVM_CLOCK_REALTIME: clk = CLOCK_REALTIME; break;
        case ORBISJVM_CLOCK_MONOTONIC: clk = CLOCK_MONOTONIC; break;
        case ORBISJVM_CLOCK_MONOTONIC_RAW: clk = CLOCK_MONOTONIC_RAW; break;
        case ORBISJVM_CLOCK_PROCESS_CPUTIME: clk = CLOCK_PROCESS_CPUTIME_ID; break;
        case ORBISJVM_CLOCK_THREAD_CPUTIME: clk = CLOCK_THREAD_CPUTIME_ID; break;
        default: return -EINVAL;
    }

    struct timespec posix_ts;
    int ret = clock_gettime(clk, &posix_ts);
    if (ret != 0) return -errno;

    ts->tv_sec = posix_ts.tv_sec;
    ts->tv_nsec = posix_ts.tv_nsec;
    return 0;
}

int orbisjvm_clock_getres(orbisjvm_clock_t clock, orbisjvm_timespec_t* res) {
    if (!res) return -1;

    clockid_t clk;
    switch (clock) {
        case ORBISJVM_CLOCK_REALTIME: clk = CLOCK_REALTIME; break;
        case ORBISJVM_CLOCK_MONOTONIC: clk = CLOCK_MONOTONIC; break;
        case ORBISJVM_CLOCK_MONOTONIC_RAW: clk = CLOCK_MONOTONIC_RAW; break;
        case ORBISJVM_CLOCK_PROCESS_CPUTIME: clk = CLOCK_PROCESS_CPUTIME_ID; break;
        case ORBISJVM_CLOCK_THREAD_CPUTIME: clk = CLOCK_THREAD_CPUTIME_ID; break;
        default: return -EINVAL;
    }

    struct timespec posix_res;
    int ret = clock_getres(clk, &posix_res);
    if (ret != 0) return -errno;

    res->tv_sec = posix_res.tv_sec;
    res->tv_nsec = posix_res.tv_nsec;
    return 0;
}

int orbisjvm_gettimeofday(orbisjvm_timeval_t* tv, orbisjvm_timezone_t* tz) {
    if (!tv) return -1;

    struct timeval posix_tv;
    struct timezone posix_tz;
    int ret = gettimeofday(&posix_tv, tz ? &posix_tz : NULL);
    if (ret != 0) return -errno;

    tv->tv_sec = posix_tv.tv_sec;
    tv->tv_usec = posix_tv.tv_usec;

    if (tz) {
        tz->tz_minuteswest = posix_tz.tz_minuteswest;
        tz->tz_dsttime = posix_tz.tz_dsttime;
    }

    return 0;
}

uint64_t orbisjvm_get_monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint64_t orbisjvm_get_monotonic_us(void) {
    return orbisjvm_get_monotonic_ns() / 1000;
}

uint64_t orbisjvm_get_monotonic_ms(void) {
    return orbisjvm_get_monotonic_ns() / 1000000;
}

uint64_t orbisjvm_get_walltime_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint64_t orbisjvm_get_walltime_us(void) {
    return orbisjvm_get_walltime_ns() / 1000;
}

uint64_t orbisjvm_get_walltime_ms(void) {
    return orbisjvm_get_walltime_ns() / 1000000;
}

/* Sleep functions */

int orbisjvm_nanosleep(const orbisjvm_timespec_t* req, orbisjvm_timespec_t* rem) {
    if (!req) return -1;

    struct timespec posix_req = { req->tv_sec, req->tv_nsec };
    struct timespec posix_rem;

    int ret = nanosleep(&posix_req, rem ? &posix_rem : NULL);
    if (ret != 0) {
        if (rem) {
            rem->tv_sec = posix_rem.tv_sec;
            rem->tv_nsec = posix_rem.tv_nsec;
        }
        return -errno;
    }

    return 0;
}

void orbisjvm_usleep(uint64_t us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

void orbisjvm_msleep(uint64_t ms) {
    orbisjvm_usleep(ms * 1000);
}

void orbisjvm_sleep(uint64_t seconds) {
    sleep(seconds);
}

/* Timer functions */

int orbisjvm_timer_create(orbisjvm_timer_t* timer,
                          orbisjvm_clock_t clock,
                          orbisjvm_timer_flags_t flags,
                          orbisjvm_timer_cb_t callback,
                          void* user_data) {
    if (!timer || !callback) return -1;

    clockid_t clk;
    switch (clock) {
        case ORBISJVM_CLOCK_REALTIME: clk = CLOCK_REALTIME; break;
        case ORBISJVM_CLOCK_MONOTONIC: clk = CLOCK_MONOTONIC; break;
        default: return -EINVAL;
    }

    struct orbisjvm_timer* t = orbisjvm_malloc(sizeof(struct orbisjvm_timer), ORBISJVM_MEM_ZERO);
    if (!t) return -ENOMEM;

    t->clock = clock;
    t->flags = flags;
    t->callback = callback;
    t->user_data = user_data;
    t->active = 0;

    struct sigevent sev = {0};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_callback_wrapper;
    sev.sigev_value.sival_ptr = t;

    int ret = timer_create(clk, &sev, &t->timer_id);
    if (ret != 0) {
        orbisjvm_free(t);
        return -errno;
    }

    *timer = t;
    return 0;
}

int orbisjvm_timer_start(orbisjvm_timer_t timer, uint64_t initial_delay, uint64_t interval) {
    if (!timer) return -1;

    struct itimerspec its = {0};
    its.it_value.tv_sec = initial_delay / 1000000000;
    its.it_value.tv_nsec = initial_delay % 1000000000;

    if (interval > 0 || (timer->flags & ORBISJVM_TIMER_PERIODIC)) {
        its.it_interval.tv_sec = interval / 1000000000;
        its.it_interval.tv_nsec = interval % 1000000000;
    }

    int ret = timer_settime(timer->timer_id, 0, &its, NULL);
    if (ret != 0) return -errno;

    timer->active = 1;
    return 0;
}

int orbisjvm_timer_stop(orbisjvm_timer_t timer) {
    if (!timer) return -1;

    struct itimerspec its = {0};
    int ret = timer_settime(timer->timer_id, 0, &its, NULL);
    if (ret != 0) return -errno;

    timer->active = 0;
    return 0;
}

int orbisjvm_timer_destroy(orbisjvm_timer_t timer) {
    if (!timer) return -1;

    orbisjvm_timer_stop(timer);
    int ret = timer_delete(timer->timer_id);
    if (ret != 0) return -errno;

    orbisjvm_free(timer);
    return 0;
}

uint64_t orbisjvm_timer_get_overrun(orbisjvm_timer_t timer) {
    if (!timer) return 0;
    return timer_getoverrun(timer->timer_id);
}

/* RTC functions (using Orbis OS RTC API when available) */

int orbisjvm_rtc_get_current(orbisjvm_rtc_timetable_t* tt) {
    if (!tt) return -1;

    /* Use wall clock as fallback */
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);

    tt->year = tm->tm_year + 1900;
    tt->month = tm->tm_mon + 1;
    tt->day = tm->tm_mday;
    tt->hour = tm->tm_hour;
    tt->minute = tm->tm_min;
    tt->second = tm->tm_sec;
    tt->microsecond = 0;

    return 0;
}

int orbisjvm_rtc_set_current(const orbisjvm_rtc_timetable_t* tt) {
    (void)tt;
    /* Requires privileges on PS4 */
    return -EPERM;
}

time_t orbisjvm_rtc_to_time_t(const orbisjvm_rtc_timetable_t* tt) {
    if (!tt) return -1;

    struct tm tm = {0};
    tm.tm_year = tt->year - 1900;
    tm.tm_mon = tt->month - 1;
    tm.tm_mday = tt->day;
    tm.tm_hour = tt->hour;
    tm.tm_min = tt->minute;
    tm.tm_sec = tt->second;
    tm.tm_isdst = -1;

    return mktime(&tm);
}

void orbisjvm_time_t_to_rtc(time_t t, orbisjvm_rtc_timetable_t* tt) {
    if (!tt) return;

    struct tm* tm = localtime(&t);
    if (!tm) {
        memset(tt, 0, sizeof(*tt));
        return;
    }

    tt->year = tm->tm_year + 1900;
    tt->month = tm->tm_mon + 1;
    tt->day = tm->tm_mday;
    tt->hour = tm->tm_hour;
    tt->minute = tm->tm_min;
    tt->second = tm->tm_sec;
    tt->microsecond = 0;
}

/* String formatting */

int orbisjvm_format_iso8601(const orbisjvm_timespec_t* ts, char* buffer, size_t buflen) {
    if (!ts || !buffer || buflen < 24) return -1;

    time_t sec = ts->tv_sec;
    struct tm* tm = gmtime(&sec);
    if (!tm) return -1;

    int written = snprintf(buffer, buflen, "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ",
                           tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                           tm->tm_hour, tm->tm_min, tm->tm_sec, ts->tv_nsec);
    return written >= 0 && (size_t)written < buflen ? 0 : -1;
}

int orbisjvm_format_rfc3339(const orbisjvm_timespec_t* ts, char* buffer, size_t buflen) {
    /* RFC 3339 is similar to ISO 8601 */
    return orbisjvm_format_iso8601(ts, buffer, buflen);
}

int orbisjvm_parse_iso8601(const char* str, orbisjvm_timespec_t* ts) {
    if (!str || !ts) return -1;

    struct tm tm = {0};
    long nsec = 0;
    int parsed = sscanf(str, "%d-%d-%dT%d:%d:%d.%ldZ",
                        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                        &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &nsec);

    if (parsed < 6) return -1;

    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    tm.tm_isdst = -1;

    time_t sec = mktime(&tm);
    if (sec == -1) return -1;

    ts->tv_sec = sec;
    ts->tv_nsec = nsec;
    return 0;
}

void orbisjvm_get_time_stats(orbisjvm_time_stats_t* stats) {
    if (!stats) return;

    stats->monotonic_freq_hz = 1000000000ULL;  /* 1 GHz = 1 ns resolution */
    stats->monotonic_is_steady = 1;

    struct timespec res;
    clock_getres(CLOCK_MONOTONIC, &res);
    stats->timer_resolution_ns = res.tv_sec * 1000000000ULL + res.tv_nsec;
}