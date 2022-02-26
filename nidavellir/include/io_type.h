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
#define esam_type_poweron               (esam_type_base+1)     //ESAM 上电初始化
#define esam_type_poweroff              (esam_type_poweron+1)  //ESAM 掉电初始化
#define esam_type_get_esam_all_info1    (esam_type_poweroff+1) //5.1.1明文直接读取批量data1（获取ESAM信息、读取钱包）
#define esam_type_get_esam_all_info2    (esam_type_get_esam_all_info1+1) //5.1.1明文直接读取批量data2（获取ESAM信息、读取钱包）
#define esam_type_get_esam_p2_info      (esam_type_get_esam_all_info2+1) //5.1.1单项获取电表安全芯片信息
#define esam_type_hes_meet_consult      (esam_type_get_esam_p2_info+1)   //5.1.2主站/掌机会话密钥协商
#define esam_type_terminal_meet_consult (esam_type_hes_meet_consult+1)   //5.1.3终端会话密钥协商
#define esam_type_plaintext_mac         (esam_type_terminal_meet_consult+1) //5.1.4.1主站/掌机/终端对电能表读取，电表计算明文+MAC
#define esam_type_ciphertext            (esam_type_plaintext_mac+1)         //5.1.4.1主站/掌机/终端对电能表读取，电表计算密文
#define esam_type_ciphertext_mac        (esam_type_ciphertext+1)            //5.1.4.1主站/掌机/终端对电能表读取，电表计算密文+MAC
#define esam_type_hes_broadcast         (esam_type_ciphertext_mac+1)        //5.1.4.2主站对电能表广播
#define esam_type_terminal_broadcast    (esam_type_hes_broadcast+1)         //5.1.4.3终端对电能表广播
#define esam_type_hes_point_get_data2   (esam_type_terminal_broadcast+1)    //5.1.4.4主站/掌机对电能表点对点交互获取DATA2
#define esam_type_hes_point_get_data4   (esam_type_hes_point_get_data2+1)   //5.1.4.4主站/掌机对电能表点对点交互获取DATA4
#define esam_type_terminal_point_get_data2   (esam_type_hes_point_get_data4+1)    //5.1.4.5终端对电能表点对点交互获取DATA2
#define esam_type_terminal_point_get_data4   (esam_type_terminal_point_get_data2+1)   //5.1.4.5终端对电能表点对点交互获取DATA4
#define esam_type_read_esam_data        (esam_type_terminal_point_get_data4+1)   //5.1.4.6(1)ESAM数据读取
#define esam_type_file_updata           (esam_type_read_esam_data+1)               //5.1.4.6(2)ESAM数据更新
#define esam_type_key_updata            (esam_type_file_updata+1)           //5.1.4.6(3)ESAM密钥更新
#define esam_type_set_other             (esam_type_key_updata+1)            //5.1.4.6(5)参数设置
#define esam_type_ir_authen_request     (esam_type_set_other+1)             //5.1.4.6(6)红外认证请求
#define esam_type_ir_authen             (esam_type_ir_authen_request+1)     //5.1.4.6(7)红外认证
#define esam_type_upreport              (esam_type_ir_authen+1)             //5.1.4.7电能表主动上报到主站/终端
#define esam_type_terminal_id_authen    (esam_type_upreport+1)              //5.1.4.8终端与电表身份认证    
#define esam_type_software_check        (esam_type_terminal_id_authen+1)    //5.1.5 软件比对





#endif
