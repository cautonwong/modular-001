#include "nidavellir.h"


/*
 * 日期合法性判定
 * */
int check_date(int year, int month, int day, int hour, int min, int sec)
{
	struct tm tm_new;

	//0xFF:表示时间无效，控制下发参数（0x8105）的起始、结束时间会有FF数据下发
	if (year == 0xFFFF || month == 0xFF || day == 0xFF || hour == 0xFF || min == 0xFF || sec == 0xFF) {
		return 0;
	}
	if (year < 1900 || month <= 0 || month > 12 || day <= 0 || day > 31
		|| hour < 0 || hour > 23 || min < 0 || min>59 || sec < 0 || sec > 59) {
		return -1;
	}
	tm_new.tm_year = year - 1900;
	tm_new.tm_mon = month - 1;
	tm_new.tm_mday = day;
	tm_new.tm_hour = hour;
	tm_new.tm_min = min;
	tm_new.tm_sec = sec;
	time_t time_new = mktime(&tm_new);
	//localtime_r(&time_new, &tm_new);
	if (tm_new.tm_year != (year - 1900) || tm_new.tm_mon != (month - 1) || tm_new.tm_mday != day) {
		return -1;
	}
	else {
		return 0;
	}
}


const unsigned int  DayTab[13] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


uint8_t swap_byte_bit(uint8_t hex)
{
    uint8_t i, value = 0;

    for(i=0; i<8;i++)
    {
        if(hex & (1<<i))
        {
            value |= (1<<(7-i));
        }
    }
    return value;
    
}


/***********************************************************************/
/*  函数名称：BCD_BYTE                                                 */
/*  函数功能：将单字节BCD码转换为单字节HEX码                           */
/*  输入参数：BCD：单字节BCD码数据                                     */
/*  输出参数：单字节HEX码数据                                          */
/***********************************************************************/
unsigned char BCD_BYTE( unsigned char BCD )
{
     return( ((BCD>>4) & 0x0F)*10 + (BCD & 0x0F) );
}



unsigned char BYTE_BCD( unsigned char HEX )
{
	if( HEX > 99 )
		return 0x00;

	return( (HEX/10)*0x10 + HEX%10 );
}

uint8_t bcd_to_hex(uint8_t bcd)
{
    uint8_t result;

    result  = bcd & 0x0F;                   /**< 个位 */
    bcd   >>= 4;
    result += bcd * 10;                     /**< 十位 */
    return result;                          /**< 返回结果 */
}


void bcd_to_hex_array(uint8_t *hexary, uint8_t *bcdary, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        hexary[i] = bcd_to_hex(bcdary[i]);
    }
}


void hex_to_bcd_array(uint8_t *hexary, uint8_t *bcdary, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        bcdary[i] = BYTE_BCD(hexary[i]);
    }
}


uint16_t word_bcd_to_hex(uint16_t bcd)
{
    Word16 *wd = (Word16 *)&bcd;            /**< 将要转化的内容分成两个字节 */
    uint16_t result;

    result = bcd_to_hex(wd->byte[0]);     /**< 个位和十位 */
    /*百位和千位 */
    result += ((uint16_t)bcd_to_hex(wd->byte[1])) * 100;
    return result;
}


uint32_t long_bcd_tohex(uint32_t bcd)
{
    Word32 *wd = (Word32 *)&bcd;            /* 将要转换的内容分成2个字 */
    uint32_t result;

    result = word_bcd_to_hex(wd->word[0]);      /* 个十百千万十万百万千万 */   
    result += word_bcd_to_hex(wd->word[1]) * 10000;
    return result;
}

bool Isbcddataarray(uint8_t *src, uint8_t len)
{
    uint8_t i;

    for (i = 0; i < len; i++)
    {
        if (((src[i] & 0x0F) > 0x09) || ((src[i] & 0xF0) > 0x90))
        {
            return false;
        }
    }

    return true;
}
/************************************************************************/
const uint8_t  gui_HexVar[7] =
{
    0x00, 0x16, 0x32, 0x48, 0x64, 0x80, 0x96,
};

uint8_t bytehex_to_bcd(uint8_t hex)
{
    uint8_t t1, t2;

    if (hex > 99)                       /* 不要超过0x99,超过的请调用函数IntHexToBcd */
    {
        return 0x99;
    }

    t1 = hex & 0x0F;                    /**< 取低4位进行转换 */

    if (t1 > 9)                         /**< 判断低4位是否需要进位 */
    {
        t1 += 6;
    }

    t2 = hex >> 4;
    t2 = gui_HexVar[t2];                /**< 获取高四位对应的BCD码的数值 */

    hex = t1;
    t1  = (hex & 0x0F) + (t2 & 0x0F);   /**< 低四位 */
    t2  = (hex & 0xF0) + (t2 & 0xF0);   /**< 高四位 */

    if (t1 > 9)                         /**< 低四位的进位判断 */
    {
        t1 += 6;
    }

    return t1 + t2;                     /**< 返回转换好的结果 */
}


uint16_t shorthex_to_bcd(uint16_t hex)
{
    uint16_t result = 0;

    if (hex > 9999)
    {
        return 0x9999;
    }

    if (hex >= 8000)
    {
        result |= 0x8000;
        hex    -= 8000;
    }

    if (hex >= 4000)
    {
        result |= 0x4000;
        hex    -= 4000;
    }

    if (hex >= 2000)
    {
        result |= 0x2000;
        hex    -= 2000;
    }

    if (hex >= 1000)
    {
        result |= 0x1000;
        hex    -= 1000;
    }

    if (hex >= 800)
    {
        result |= 0x800;
        hex    -= 800;
    }

    if (hex >= 400)
    {
        result |= 0x400;
        hex    -= 400;
    }

    if (hex >= 200)
    {
        result |= 0x200;
        hex    -= 200;
    }

    if (hex >= 100)
    {
        result |= 0x100;
        hex    -= 100;
    }

    result |= (uint16_t)bytehex_to_bcd((uint8_t)hex);

    return result;
}
/***********************************************************************/
/*  函数名称：GetTotalDay                                              */
/*  函数功能：根据年月日计算总天数                                     */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：总天数                                                   */
/***********************************************************************/
unsigned int GetTotalDay( unsigned char* Addr )
{
  	unsigned int Day,Mon,Year;

	Year= ( *(Addr+2) );
	Mon = ( *(Addr+1) );
	Day = ( *Addr );
	Day = Day + DayTab[Mon] + Year*365 + (Year>>2) + 1;
	if( ((Year&0x03) == 0) && (Mon <= 2) )
		Day--;
   	return Day;
}

/***********************************************************************/
/*  函数名称：GetWeek                                                  */
/*  函数功能：根据年月日计算周                                         */
/*  输入参数：Addr：年月日数据地址                                     */
/*  输出参数：周                                                       */
/***********************************************************************/
unsigned char GetWeek( unsigned char* Addr )
{
	unsigned char Week;

	Week = (unsigned char)(( GetTotalDay( Addr ) + 5 ) % 7);
	return Week;
}

void WORD_BCD2( unsigned char *BCD, unsigned int WORD )
{
	if( WORD > 9999 )
	{
		*BCD = 0x00;
		*(BCD+1) = 0x00;
		return;
	}

	*(BCD+1) = BYTE_BCD( (unsigned char)(WORD/100) );
	*BCD = BYTE_BCD( (unsigned char)(WORD%100) );
}


void HEX_BCD( unsigned char* BCD, unsigned long HEX )
{
	unsigned char i;

	if( HEX > 99999999 )
	{
		*BCD = 0x00;
		*(BCD+1) = 0x00;
		*(BCD+2) = 0x00;
		*(BCD+3) = 0x00;
	}
	else
	{
		for( i=0;i<4;i++ )
		{
			*BCD = (unsigned char)(HEX%10);
	        	HEX = HEX/10;
	        	*BCD += (unsigned char)((HEX%10)<<4) & 0xF0;
	        	HEX = HEX/10;
	        	BCD++;
		}
	}
}


void HEX_BCD_Signed(int32_t hex , uint8_t *dest , uint8_t len)
{
    uint32_t ret=0; 
    uint32_t bit_space[3] = {0x00008000,0x00800000,0x80000000};
    uint32_t tmp = 0;
    if(hex < 0)
    {
        hex = ~hex + 1;
        if((len>1)&&(len<5))
        {
            ret = bit_space[len-2];
        }
    }
    HEX_BCD( (uint8_t *)&tmp, hex );
    ret |= tmp;
    ndv_memcopy(dest , (uint8_t*)&ret , len);
}

uint16_t randomize_int(uint16_t interval)
{
	static uint32_t seed = 1;
	uint32_t hi, lo;
	lo = 16807 * (seed & 0xFFFF);
	hi = 16807 * (seed >> 16);
	lo += (hi & 0x7FFF) << 16;
	lo += hi >> 15;
	if (lo > 0x7FFFFFFF) lo -= 0x7FFFFFFF;
	seed = lo;
	return (uint16_t)( ((float)interval * (float)lo) / (float)0x7FFFFFFF );
}

float bound_min_max(float val, float min, float max)
{
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

/* Test for leap year. Nonzero if YEAR is a leap year (every 4 years,
 except every 100th isn't, and every 400th is). */
# define __isleap(year) \
 ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

/* How many days come before each month (0-12).  */
static const uint16_t __mon_yday[2][13] = {
	/* Normal years.  */
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
	/* Leap years.  */
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

#define	SECS_PER_HOUR	(60 * 60)
#define	SECS_PER_DAY	(SECS_PER_HOUR * 24)


/**
 * Convert UNIX timestamp to date/time
 * Based on code from GNU C Library
 */
void date_from_timestamp(uint32_t timestamp, DateTimeT *date_time)
{
	uint16_t y;
	int16_t days;
	uint32_t rem;

	days = timestamp / SECS_PER_DAY;
	rem = timestamp % SECS_PER_DAY;

	date_time->hour = rem / SECS_PER_HOUR;
	rem %= SECS_PER_HOUR;
	date_time->min = rem / 60;
	date_time->sec = rem % 60;
	/* January 1, 1970 was a Thursday.  */
	date_time->wday = (4 + days) % 7;
	y = 1970;

#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))

	while (days < 0 || days >= (__isleap (y) ? 366 : 365))
	{
		/* Guess a corrected year, assuming 365 days per year.  */
		uint16_t yg = y + days / 365 - (days % 365 < 0);

		/* Adjust DAYS and Y to match the guessed year.  */
		days -= ((yg - y) * 365
				 + LEAPS_THRU_END_OF (yg - 1)
				 - LEAPS_THRU_END_OF (y - 1));
		y = yg;
	}
	date_time->year = y - 1900;
	if (date_time->year != y - 1900)
	{
		/* The year cannot be represented due to overflow.  */
		return;
	}
	const uint16_t *ip = __mon_yday[__isleap(y)];
	for (y = 11; days < (uint16_t)ip[y]; --y){
		continue;
	}
	days -= ip[y];
	date_time->mon = y;
	date_time->mday = days + 1;
}

uint16_t buf_to_uint16_msb(uint8_t* buf)
{
	return ((uint16_t)buf[0] << 8) + buf[1];
}

uint16_t buf_to_uint16_lsb(uint8_t *buf)
{
    uint16_t data = 0;

    data = ((uint16_t)buf[1] << 8) + buf[0];
    return data;
}

uint8_t uint16_to_buf_lsb(uint16_t data, uint8_t *buf)
{
    buf[0] = (uint8_t)data;
    buf[1] = (uint8_t)(data >> 8);
    return 2;
}

uint8_t uint16_to_buf_msb(uint16_t data, uint8_t *buf)
{
    buf[0] = (uint8_t)(data >> 8);
    buf[1] = (uint8_t)data;
    return 2;
}

uint32_t buf_to_uint32_msb(uint8_t* buf)
{
	return ((uint32_t)buf[0] << 24) + ((uint32_t)buf[1] << 16) + ((uint32_t)buf[2] << 8) + buf[3];
}

uint8_t uint32_to_buf_msb(uint32_t data, uint8_t* buf)
{
	buf[3] = (uint8_t)data;
	buf[2] = (uint8_t)(data >> 8);
	buf[1] = (uint8_t)(data >> 16);
	buf[0] = (uint8_t)(data >> 24);
	return 4;
}

uint8_t uint32_to_buf_lsb(uint32_t data, uint8_t* buf)
{
	buf[0] = (uint8_t)data;
	buf[1] = (uint8_t)(data >> 8);
	buf[2] = (uint8_t)(data >> 16);
	buf[3] = (uint8_t)(data >> 24);
	return 4;
}

uint32_t buf_to_uint32_lsb(uint8_t* buf)
{
	uint32_t data = 0;
	data = ((uint32_t)buf[3] << 24) +
		((uint32_t)buf[2] << 16) +
		((uint32_t)buf[1] << 8) +
		buf[0];
	return data;
}

void swap_buffer(uint8_t *buff, uint16_t length)
{
    uint8_t  uc_temp;
    uint16_t ui_len;

    // 交换的长度应该是所有数据的一半 
    for (ui_len = 0; ui_len < (length >> 1); ui_len++)
    {
        uc_temp = buff[ui_len];             

        buff[ui_len] = buff[length - ui_len - 1];
        buff[length - ui_len - 1] = uc_temp;
    }
}

void mem_swap_cpy(void* dest, void* src, unsigned short length)
{
	unsigned short i;
	unsigned char* ptrdest = (unsigned char*)dest, * ptrsrc = (unsigned char*)src;

	for (i = 0; i < length; i++)
	{
		*(ptrdest + i) = *(ptrsrc + length - 1 - i);
	}
}

void ndv_memcopy(void *tb, void *sb, uint16_t length)
{
    uint8_t *tbuff = tb;
    uint8_t *sbuff = sb;

    // 先判断是否有正向移动重叠区域 
    if ((sbuff < tbuff) && (sbuff + length > tbuff))
    {
        // 有就从反向移动 
        while (length != 0)                 // 根据长度进行移动 
        {
            length--;
            tbuff[length] = sbuff[length];  // 移动 
        }
    }
    else                                    // 其它情况进行正向移动 
    {
        while (length != 0)
        {
            length--;
            *tbuff++ = *sbuff++;            // 移动 
        }
    }
}
void ndv_move_buffer(uint8_t *sbuff, uint8_t *tbuff, uint16_t length)
{
    /* 先判断是否有正向移动重叠区域 */
    if ((sbuff < tbuff) && (sbuff + length > tbuff))
    {
        /* 有就从反向移动 */
        while (length != 0)                 /**< 根据长度进行移动 */
        {
            length--;
            tbuff[length] = sbuff[length];  /**< 移动 */
        }
    }
    else                                    /**< 其它情况进行正向移动 */
    {
        while (length != 0)
        {
            length--;
            *tbuff++ = *sbuff++;            /**< 移动 */
        }
    }
}

uint8_t mem_compare(void* data1, void* data2, uint16_t length)
{
	unsigned char* buff1;
	unsigned char* buff2;

	buff1 = (unsigned char*)data1;
	buff2 = (unsigned char*)data2;

	while (length > 0)
	{
		length--;

		if (*buff1 != *buff2)
		{
			if (*buff1 > *buff2)
			{
				return 1;
			}
			else
			{
				return 2;
			}
		}

		buff1++;
		buff2++;
	}

	return 0;
}

uint8_t BCD_Check( uint8_t* Addr, uint8_t Len )
{
	while( Len > 0 )
    {
	    if( (*Addr&0x0F) > 0x09 ) return 1;
	    if( (*Addr&0xF0) > 0x90 ) return 1;
	    Addr++;
	    Len--;
    }
	return 0;
}

static const uint16_t crc16_tab[256] =
{
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

uint16_t crc16(uint16_t init, uint8_t* buf, size_t len)
{
    uint16_t cs = init;/**<  0xffff; */

    while (len--)
    {
        cs = (cs >> 8) ^ crc16_tab[(cs ^ *buf++) & 0xff];
    }
    cs ^= 0xffff;
    return cs;
}

uint16_t nd_get_crc16(uint8_t* buf, size_t len)
{
    uint16_t cs = 0xffff;

    while (len--)
    {
        cs = (cs >> 8) ^ crc16_tab[(cs ^ *buf++) & 0xff];
    }
    cs ^= 0xffff;
    return cs;
}

uint16_t update_crc16(uint16_t init, uint8_t byte)
{
    uint16_t cs = init;/**<  0xffff; */

    cs = (cs >> 8) ^ crc16_tab[(cs ^ byte) & 0xff];
    //cs ^= 0xffff;

    return cs;
}