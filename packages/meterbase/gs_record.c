#include "nidavellir.h"
#include "applications.h"



// 上电运行期间 每秒的调用 
int record_judge(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	ia_over_event_judge(instance, pipeline, mesg);
}


