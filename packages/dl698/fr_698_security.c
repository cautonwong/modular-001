#include "nidavellir.h"
#include "fr_698.h"


int fr_security_decode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
    struct fifo_t* fifo = (struct fifo_t*)pipeline->in_queue;
    uint32_t addr;
    uint8_t buf[100],len=0;
    
    fifoBuf_getData(fifo, buf, len);
/*    
    if( (Temp08 == 1) || (Temp08 == 2) ) //RN(1)、RN_MAC(2)
    {
        
    }
    else    //SID(3)、SID_MAC(0)
    {
        //SID(4)
        //附加信息（2）
    }
    
    //明文/密文
    memcpy(buf, len);

	//MAC
	if( (Temp08 != 1) && (Temp08 != 3) )  //SID_MAC、RN_MAC
	{
		memcpy(4);
	}
*/    
    write_esam(addr , buf , len);
  
	return 1;
}


int fr_security_encode(struct instance_t* instance, struct pipeline_t* pipeline, struct message_t* mesg)
{
	return 1;
}