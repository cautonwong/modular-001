#ifndef ND_TARGET_STATEGRID_PIDS_H
#define ND_TARGET_STATEGRID_PIDS_H

/* Protocol channel ids wired by the product target. The numeric values are
 * the legacy pipeline ids the DL/T 698 and DL/T 645 adapters already use. */
enum stategrid_pid_t
{
    STG_PID_KERNEL = 13,
    STG_PID_RX_698 = 14,
    STG_PID_METER_BASE = 22,
    STG_PID_RESOURCE_MANAGER = 23,
    STG_PID_PLC_RX_698 = 24,
    STG_PID_IR_RX_698 = 26,
    STG_PID_LOWPOWER_WAKEUP = 30,
    STG_PID_RX_645 = 60,
    STG_PID_TX_645 = 61,
};

#endif
