#ifndef __INCLUDE_UTILS_H__
#define __INCLUDE_UTILS_H__

typedef struct {
	uint8_t sec;  // seconds after the minute - [0, 60]
	uint8_t min;  // minutes after the hour - [0, 59]
	uint8_t hour; // hours since midnight - [0, 23]
	uint8_t mday; // day of the month - [1, 31]
	uint8_t mon;  // months since January - [0, 11]
	uint8_t wday; // days since Sunday - [0, 6]
	uint8_t year; // years since 1900
} DateTimeT;
uint16_t randomize_int(uint16_t interval);
float bound_min_max(float val, float min, float max);
void date_from_timestamp(uint32_t timestamp, DateTimeT *date_time);
void rtc_tm_normalize(struct tm *time);

int rtc_tm_compare(const struct tm *a, const struct tm *b);

/**
 * @brief Convert time struct into timestamp.
 *
 * @pre   The time structs @p a and @p b are assumed to be normalized.
 *        Use @ref rtc_tm_normalize to normalize a struct tm that has been
 *        manually edited.
 *
 * @param[in] t       The time struct to convert
 *
 * @return            elapsed seconds since `RIOT_EPOCH`
 */
uint32_t rtc_mktime(struct tm *t);

/**
 * @brief Converts an RTC timestamp into a  time struct.
 *
 * @param[in]  time   elapsed seconds since `RIOT_EPOCH`
 * @param[out] t      the corresponding timestamp
 */
void rtc_localtime(uint32_t time, struct tm *t);

/**
 * @brief Verify that a time struct @p t contains valid data.
 *
 * @note    This function checks whether the fields of the
 *          struct @p t are positive and within the bounds set
 *          by @ref rtc_tm_normalize.
 *
 * @param[in] t       The struct to be checked.
 *
 * @return            true when valid, false if not
 */
bool rtc_tm_valid(const struct tm *t);

int time_to_array_0(struct tm *time, uint8_t* buf, size_t len);
int array_0_to_time(struct tm* time, uint8_t* buf, size_t len);
uint32_t buf_to_uint32_msb(uint8_t* buf);
uint16_t buf_to_uint16_msb(uint8_t* buf);

uint8_t uint32_to_buf_msb(uint32_t data, uint32_t* buf);
uint8_t uint32_to_buf_lsb(uint32_t data, uint8_t* buf);
uint32_t buf_to_uint32_lsb(uint8_t* buf);
uint8_t mem_compare(void* data1, void* data2, uint16_t length);
void mem_swap_cpy(void* dest, void* src, unsigned short length);

int hex2bin(char* dst, const char* src, uint16_t count);

#endif
