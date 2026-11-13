#ifndef ND_KERNEL_CORE_H
#define ND_KERNEL_CORE_H
#include "types.h"
#include "object.h"
#include "channel.h"
#include "message.h"
#include "instance.h"
#include "pids.h"
#include "resource_ids.h"
#include "tasklet.h"
#include "rtc.h"
#include "log_object.h"
#include "device.h"
#include "environment.h"
#include "ipc_globals.h"
#include "serial.h"
#include "work_type.h"
#include "resource_map.h"
#include "flags.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#ifdef WIN32
#include <windows.h>
#ifdef WIN32_U_TEST
#include "test_utils.h"
#include "u_test.h"
#endif
#include "log.h"
#endif

#include "mlog.h"

#define  DEBUG

#include <stdlib.h>
#include "time.h"
#include "event_flag.h"

#include "utils.h"
#include "list.h"
#include "fifo.h"
#include "pipeline.h"
#include "memory.h"
#include "infra/kernel/resource_blocks.h"
#include "infra/kernel/key_config.h"
#ifndef WIN32
#endif

/*
组织成多个pipeline,每个pipeline由多个filter组成，
pipeline之间由pipe联接,实现ipc
*/


#endif
