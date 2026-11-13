#include "nidavellir.h"
#include "threadx/tx_api.h"
#include <stdio.h>
#include "cpu_surport.h"
#include "application/apps/app_entry.h"

#define     DEMO_STACK_SIZE         1024
#define     DEMO_BYTE_POOL_SIZE     9120
#define     DEMO_BLOCK_POOL_SIZE    100
#define     DEMO_QUEUE_SIZE         100

/* Define the ThreadX object control blocks...  */

TX_THREAD               thread_0;
TX_THREAD               thread_1;
TX_THREAD               thread_2;
TX_THREAD               thread_3;
TX_THREAD               thread_4;
TX_THREAD               thread_5;
TX_THREAD               thread_6;
TX_THREAD               thread_7;
TX_QUEUE                queue_0;
TX_SEMAPHORE            semaphore_0;
TX_SEMAPHORE            semaphore_1;//块设备信号量
TX_MUTEX                mutex_0;
TX_EVENT_FLAGS_GROUP    event_flags_0;
TX_EVENT_FLAGS_GROUP    event_flags_1;
TX_EVENT_FLAGS_GROUP    event_flags_2;//唤醒IO线程

TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;


/* Define the counters used in the demo application...  */

ULONG           thread_0_counter;
ULONG           thread_1_counter;
ULONG           thread_1_messages_sent;
ULONG           thread_2_counter;
ULONG           thread_2_messages_received;
ULONG           thread_3_counter;
ULONG           thread_4_counter;
ULONG           thread_5_counter;
ULONG           thread_6_counter;
ULONG           thread_7_counter;
ULONG           thread_8_counter;


/* Define thread prototypes.  */

void    thread_0_entry(ULONG thread_input);
void    thread_1_entry(ULONG thread_input);
void    thread_2_entry(ULONG thread_input);
void    thread_3_and_4_entry(ULONG thread_input);
void    thread_meter_base_entry(ULONG thread_input);
void    thread_persistent_entry(ULONG thread_input);
void    thread_6_and_7_entry(ULONG thread_input);
extern struct pipeline_t pl_meter_base;
extern struct pipeline_t pl_rx_698;
extern struct instance_t* baremetal_instance_entry_main(int argc, char** argv);


/* Define main entry point.  */
int main()
{

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}

/* Define what the initial system looks like.  */
void  tx_application_define(void* first_unused_memory)
{

    CHAR* pointer = TX_NULL;
    struct instance_t *instance = baremetal_instance_entry_main(0, NULL);

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", first_unused_memory, DEMO_BYTE_POOL_SIZE);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */

       /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,
        pointer, DEMO_STACK_SIZE,
        1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);


    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 1 and 2. These threads pass information through a ThreadX
       message queue.  It is also interesting to note that these threads have a time
       slice.  */
    tx_thread_create(&thread_1, "thread 1", thread_1_entry, 1,
        pointer, DEMO_STACK_SIZE,
        16, 16, 4, TX_AUTO_START);

    /* Allocate the stack for thread 2.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_2, "thread 2", thread_2_entry, 2,
        pointer, DEMO_STACK_SIZE,
        16, 16, 4, TX_AUTO_START);

    /* Allocate the stack for thread 3.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 3 and 4.  These threads compete for a ThreadX counting semaphore.
       An interesting thing here is that both threads share the same instruction area.  */
    tx_thread_create(&thread_3, "thread 3", thread_3_and_4_entry, (ULONG)instance,
        pointer, DEMO_STACK_SIZE,
        8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 4.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_4, "thread 4", thread_3_and_4_entry, (ULONG)instance,
        pointer, DEMO_STACK_SIZE,
        8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 5.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create thread 5.  This thread simply pends on an event flag which will be set
       by thread_0.  */
    tx_thread_create(&thread_5, "thread 5", thread_meter_base_entry, (ULONG)instance,
        pointer, DEMO_STACK_SIZE,
        4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 6.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 6 and 7.  These threads compete for a ThreadX mutex.  */
    tx_thread_create(&thread_6, "thread 6", thread_persistent_entry, (ULONG)instance,
        pointer, DEMO_STACK_SIZE,
        8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the message queue.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT);

    /* Create the message queue shared by threads 1 and 2.  */
    tx_queue_create(&queue_0, "queue 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE * sizeof(ULONG));

    /* Create the semaphore used by threads 3 and 4.  */
    tx_semaphore_create(&semaphore_0, "semaphore 0", 1);
    tx_semaphore_create(&semaphore_1, "semaphore 1", 1);
    tx_event_flags_create(&event_flags_1, "event flags 1");
    tx_event_flags_create(&event_flags_2, "event flags 2");
    /* Create the event flags group used by threads 1 and 5.  */
    tx_event_flags_create(&event_flags_0, "event flags 0");


    /* Create the mutex used by thread 6 and 7 without priority inheritance.  */
    tx_mutex_create(&mutex_0, "mutex 0", TX_NO_INHERIT);

    /* Allocate the memory for a small block pool.  */
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_BLOCK_POOL_SIZE, TX_NO_WAIT);

    /* Create a block memory pool to allocate a message buffer from.  */
    tx_block_pool_create(&block_pool_0, "block pool 0", sizeof(ULONG), pointer, DEMO_BLOCK_POOL_SIZE);

    /* Allocate a block and release the block memory.  */
    tx_block_allocate(&block_pool_0, (VOID**)&pointer, TX_NO_WAIT);

    /* Release the block back to the pool.  */
    tx_block_release(pointer);
}

/* Define the test threads.  */
void    thread_0_entry(ULONG thread_input)
{
    UINT    status;
    while (1)
    {
        /* Increment the thread counter.  */
        thread_0_counter++;

        /* Print results.  */
        printf("**** ThreadX Win32 Demonstration **** (c) 1996-2020 Microsoft Corporation\n\n");
        printf("           thread 0 events sent:          %lu\n", thread_0_counter);
        printf("           thread 1 messages sent:        %lu\n", thread_1_counter);
        printf("           thread 2 messages received:    %lu\n", thread_2_counter);
        printf("           thread 3 obtained semaphore:   %lu\n", thread_3_counter);
        printf("           thread 4 obtained semaphore:   %lu\n", thread_4_counter);
        printf("           thread 5 events received:      %lu\n", thread_5_counter);
        printf("           thread 6 mutex obtained:       %lu\n", thread_6_counter);
        printf("           thread 7 mutex obtained:       %lu\n\n", thread_7_counter);
        printf("           thread 8 obtained semaphore:   %lu\n", thread_8_counter);

        /* Sleep for 10 ticks.  */
        tx_thread_sleep(10);

        /* Set event flag 0 to wakeup thread 5.  */
        status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}

//数据保存线程  费时且优先级低
//1.ram数据同步到外设(定时/定量/停电)
//2.数据快照存储服务
void thread_persistent_entry(ULONG thread_input)
{
    ULONG   actual_flags;
    UINT    status;
    struct message_t mesg;
    struct instance_t *instance = (struct instance_t *)thread_input;
    struct pipeline_t *pipeline = &pl_rx_698;  
    ULONG   received_message;

    while (1)
    {
        /* Increment the thread counter.  */
        thread_8_counter++;
        //有数据保存请求
        //status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);
        //if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
           // break;
        /* Otherwise, all is okay.  Increment the received message count.  */
        //thread_2_messages_received++;

        status = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
            break;

        //获取设备访问权限  移至资源管理器中
        status = tx_semaphore_get(&semaphore_1, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
            break;

        //开始执行
        while (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
        {
            //从FIFO中取要保存的数据标识(数据块/事件记录/负荷曲线等) 打包数据 写入外部设备
            break;
        }

        status = tx_semaphore_put(&semaphore_1);
        if (status != TX_SUCCESS)
            break;

        status = tx_mutex_put(&mutex_0);
        if (status != TX_SUCCESS)
            break;
    }
}

void thread_1_entry(ULONG thread_input)
{
    UINT    status;
    while (1)
    {

        /* Increment the thread counter.  */
        thread_1_counter++;

        /* Send message to queue 0.  */
        status = tx_queue_send(&queue_0, &thread_1_messages_sent, TX_WAIT_FOREVER);

        /* Check completion status.  */
        if (status != TX_SUCCESS)
            break;

        /* Increment the message sent.  */
        thread_1_messages_sent++;
    }
}


void    thread_2_entry(ULONG thread_input)
{
    ULONG   received_message;
    UINT    status;

    while (1)
    {
        thread_2_counter++;

        /* Retrieve a message from the queue.  */
        status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);

        /* Check completion status and make sure the message is what we
           expected.  */
        if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
            break;

        /* Otherwise, all is okay.  Increment the received message count.  */
        thread_2_messages_received++;
    }
}

//通信任务
void thread_3_and_4_entry(ULONG thread_input)
{
    ULONG   actual_flags;
    UINT    status;
    struct message_t mesg;
    struct instance_t *instance = (struct instance_t *)thread_input;
    struct pipeline_t *pipeline = &pl_rx_698;
    while (1)
    {
        status = tx_event_flags_get(&event_flags_1, 0x1, TX_OR_CLEAR,
            &actual_flags, TX_WAIT_FOREVER);

        if ((status != TX_SUCCESS) || (actual_flags != 0x1))
            break;

        //先等待数据生产完成  移至资源管理器中
        status = tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
            break;

        //开始执行
        while (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
        {
            app_698_rx_entry(instance, pipeline,&mesg);
        }

        status = tx_semaphore_put(&semaphore_0);
        if (status != TX_SUCCESS)
            break;
    }
}

//基表应用入口  生产数据  
//1秒唤醒一次  
//高优先级任务  执行要快 在此线程中不进行IO等费时的操作
//event_flags_0  用于执行触发唤醒
//semaphore_0  执行过程中不切换线程 保证ctrl_block数据的一致性 用于跟通信等别的线程的数据访问同步 
void    thread_meter_base_entry(ULONG thread_input)
{
    UINT    status;
    ULONG   actual_flags;
    struct message_t mesg;
    struct instance_t *instance = (struct instance_t *)thread_input;
    struct pipeline_t *pipeline = &pl_meter_base;

    while (1)
    {

        /* Increment the thread counter.  */
        thread_5_counter++;

        /* Wait for event flag 0.  */
        //第3个参数支持如下四种参数
        //TX_AND 等待第2个参数所有请求置1的bit全部被置1。
        //TX_AND_CLEAR 等待第2个参数所有请求置1的bit全部被设置，之后相应的bit将被清零。
        //TX_OR 等待第2个参数所有请求置1的bit中任何一个bit被置1。
        //TX_OR_CLEAR 等待第2个参数所有请求置1的bit中任何一个bit被置1，之后相应的bit将被清零。
        //4.第4个参数是32bit事件标志的实际数值（未被清零前的数值）。
        //5.第5个参数是等待选项
        //TX_NO_WAIT (0x00000000)，表示不管是否满足等待条件，立即返回。如果在定时器组，初始化或中断里面调用，必须要设置成这个参数。
        //TX_WAIT_FOREVER (0xFFFFFFFF)，表示永久等待，直到满足等待条件。
        //等待时间，范围0x00000001 到 0xFFFFFFFE，单位系统时钟节拍
        //注意事项：
            //1.可以在初始化，任务，定时器组和中断服务程序里面调用。
            //2.这个函数的第3个参数推荐配置为TX_AND_CLEAR或者TX_OR_CLEAR，否则用户满足条件时，函数tx_event_flags_get会一直成立
        //
        status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR,
            &actual_flags, TX_WAIT_FOREVER);

        /* Check status.  */
        if ((status != TX_SUCCESS) || (actual_flags != 0x1))
            break;
        
        status = tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
            break;

        while (fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t))
        {
            //不发生堵塞调用  比如io操作和设备操作
            app_meter_base_entry(instance, pipeline,&mesg);
        }

        status = tx_semaphore_put(&semaphore_0);
        if (status != TX_SUCCESS)
            break;
    }
}

void example_semaphore_1(ULONG thread_input)
{
    UINT    status;
    ULONG   actual_flags;

    while (1)
    {

        status = tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);
        if (status != TX_SUCCESS)
            break;

        status = tx_semaphore_put(&semaphore_0);
        if (status != TX_SUCCESS)
            break;
    }
}
