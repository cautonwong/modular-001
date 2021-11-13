#ifndef __FR_698_COMM_H__
#define __FR_698_COMM_H__


#ifndef U_TEST
#define fr_698_get nd_get
#define fr_698_set nd_set
#define fr_698_action __nd_action
#else
#define fr_698_get __nd_get
#define fr_698_set __nd_set
#define fr_698_action __nd_action
#endif


#endif //__FR_698_COMM_H__