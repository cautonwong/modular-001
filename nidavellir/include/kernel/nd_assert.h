///@file     hfp_assert.h
///@brief    nd断言对外接口头文件
///@author   作者名
///@version  2.0.0
///@date     2019-07-20


#ifndef _ND_ASSERT_H_
#define _ND_ASSERT_H_

#ifdef __cplusplus
extern "C" {
#endif

	//extern void on_assert_fail(const uint8 *file, uint16 line);

	//#ifdef DEBUG   
	//#define nd_assert(expr)    		do{ if(!(expr)) on_assert_fail((const uint8 *)__FILE__, (uint16)__LINE__); } while(0)
	//#define nd_size_assert(i, num)		((void)sizeof(char[1 - 2 * !!(sizeof(i) != sizeof(num))]))
	//#else
	#define nd_assert(expr)    
	#define nd_size_assert(i, num) 
	//#endif

#ifdef __cplusplus
}
#endif

#endif
