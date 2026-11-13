#ifndef ND_KERNEL_PIDS_H
#define ND_KERNEL_PIDS_H

#define msta_rx_from_meter   0
#define msta_tx_to_meter   1
#define msta_rx_from_bt   2
#define msta_tx_to_bt   3
#define msta_rx_from_sta   4
#define msta_tx_to_sta   5
#define bt_rx_from_msta   6
#define bt_tx_to_msta   7
#define sta_rx_from_msta   8
#define sta_tx_to_msta   9
#define meter_rx_from_msta   10
#define meter_tx_to_msta   11
#define pid_msta   12
#define kernel   13
#define pid_rx_698   14
#define pid_tx_698   15
#define pid_rx_from_serial 16
#define pid_tx_to_serial 17
#define pid_client_rx_698   18
#define pid_client_tx_698   19
#define pid_server_rx_dlms   20
#define pid_server_tx_dlms   21
#define pid_meter_base       22
#define pid_resource_manager 23
#define pid_plc_rx_698   24
#define pid_plc_tx_698   25
#define pid_ir_rx_698    26
#define pid_ir_tx_698    27
#define pid_4852_rx_698    28
#define pid_4852_tx_698    29
#define pid_lowpower_wakeup 30

#define pid_rx_645   60
#define pid_tx_645   61

#endif
