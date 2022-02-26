#ifndef __io_type_h__
#define __io_type_h__

#define io_generic_base 0
#define io_open (io_generic_base + 1)
#define io_read (io_generic_base + 2)
#define io_write (io_generic_base + 3)
#define io_erase_sec (io_generic_base + 4)
#define io_set_callback (io_generic_base + 5)
#define io_info_get (io_generic_base + 6)
#define io_suspend (io_generic_base + 7)
#define io_close (io_generic_base + 8)
#define io_low_power (io_close + 9)
//
#define io_generic_end (io_generic_base + 0x10)

#define uart_io_type_base (io_generic_end)           //uart请求类型开始
#define uart_rx_buffer_set (uart_io_type_base + 1)   //设置uart的接收缓冲区请求类型
#define uart_tx_buffer_set (uart_io_type_base + 2)   //设置uart的发送缓冲区请求类型
#define uart_state_set (uart_io_type_base + 3)       //设置uart的控制状态请求类型 比如波特率/全半双工工作模式/dma-中断-轮询
#define uart_state_get (uart_io_type_base + 4)       //获取uart的控制状态请求类型
#define uart_rx_callback_set (uart_io_type_base + 5) //设置uart的接收回调
#define uart_tx_callback_set (uart_io_type_base + 6) //设置uart的发送回调
//在这里添加uart相关的irp type
#define uart_io_type_end (uart_io_type_base + 0x10)  //uart请求类型结束

#define io_type_base     (uart_io_type_end)
#define io_set_on        (io_type_base+1)
#define io_set_off       (io_type_base+2)
#define io_set_toggle    (io_type_base+3)
#define io_set_pulse     (io_type_base+4)
#define io_set_blink     (io_type_base+5)
#define io_get_on_off    (io_type_base+6)
#define io_type_end      (io_type_base+010)        //IO请求类型结束

#define esam_type_base                  (io_type_end)
#define esam_type_poweron               (esam_type_base+1)
#define esam_type_poweroff              (esam_type_poweron+1)
#define esam_type_get_esam_all_info1    (esam_type_poweroff+1)
#define esam_type_get_esam_all_info2    (esam_type_get_esam_all_info1+1)
#define esam_type_get_esam_p2_info      (esam_type_get_esam_all_info2+1)
#define esam_type_hes_meet_consult      (esam_type_get_esam_p2_info+1)
#define esam_type_terminal_meet_consult (esam_type_hes_meet_consult+1)
#define esam_type_plaintext_mac         (esam_type_terminal_meet_consult+1)
#define esam_type_ciphertext            (esam_type_plaintext_mac+1)
#define esam_type_ciphertext_mac        (esam_type_ciphertext+1)
#define esam_type_hes_broadcast         (esam_type_ciphertext_mac+1)
#define esam_type_terminal_broadcast    (esam_type_hes_broadcast+1)
#define esam_type_hes_point_get_data2   (esam_type_terminal_broadcast+1)
#define esam_type_read_esam_data        (esam_type_hes_point_get_data2+1)
#define esam_type_set_sid_mac           (esam_type_read_esam_data+1)
#define esam_type_set_sid               (esam_type_set_sid_mac+1)
#define esam_type_file_updata           (esam_type_set_sid+1)
#define esam_type_key_updata            (esam_type_file_updata+1)
#define esam_type_set_other             (esam_type_key_updata+1)
#define esam_type_ir_authen_request     (esam_type_set_other+1)
#define esam_type_ir_authen             (esam_type_ir_authen_request+1)
#define esam_type_upreport              (esam_type_ir_authen+1)
#define esam_type_terminal_id_authen    (esam_type_upreport+1)
#define esam_type_software_check        (esam_type_terminal_id_authen+1)





#endif
