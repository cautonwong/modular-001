#ifndef __test_app_698_server_uniformity_H__
#define __test_app_698_server_uniformity_H__

//数据链路层合法性
/////////////////////////////////////////////////////////////
//读取一个对象属性（GET）数据链路层测试用例

//1 客户机发起正确GET请求测试 如读取读取安全模式参数
void test_app_698_server_LINK_get_1_ok(void** state);
//2 错误帧起始符测试
void test_app_698_server_LINK_get_2_header_err(void** state);
//3 错误长度测试
void test_app_698_server_LINK_get_3_len_err(void** state);
//4 错误控制域测试
void test_app_698_server_LINK_get_4_ctrl_err(void** state);
//5 错误通信地址测试
void test_app_698_server_LINK_get_5_addr_err(void** state);
//6 错误客户机地址测试
void test_app_698_server_LINK_get_6_CA_err(void** state);
//7 错误逻辑地址测试
void test_app_698_server_LINK_get_7_logic_addr_err(void** state);
//8 错误HCS帧头校验测试
void test_app_698_server_LINK_get_8_hcs_err(void** state);
//9 错误GET-Request测试
void test_app_698_server_LINK_get_9_err(void** state);
//10 错误属性测试
void test_app_698_server_LINK_get_10_attribute_err(void** state);
//11 错误FCS帧校验测试
void test_app_698_server_LINK_get_11_fcs_err(void** state);
//12 结束字符测试
void test_app_698_server_LINK_get_12_end(void** state);


///////////////////////////////////////////////////////////////////////
//设置一个对象属性（SET）数据链路层测试用例
//1、客户机发起正确SET请求测试
void test_app_698_server_LINK_set_1_ok(void** state);
//2、错误帧起始符测试
void test_app_698_server_LINK_set_2_header_err(void** state);
//3、错误长度测试
void test_app_698_server_LINK_set_3_len_err(void** state);
//4、错误控制域测试
void test_app_698_server_LINK_set_4_ctrl_err(void** state);
//5、错误通信地址测试
void test_app_698_server_LINK_set_5_addr_err(void** state);
//6、错误客户机地址测试
void test_app_698_server_LINK_set_6_CA_err(void** state);
//7、错误逻辑地址测试
void test_app_698_server_LINK_set_7_logic_addr_err(void** state);
//8、错误HCS帧头校验测试
void test_app_698_server_LINK_set_8_hcs(void** state);
//9、错误SET-Request测试
void test_app_698_server_LINK_set_9_err(void** state);
//10、错误FCS帧校验测试
void test_app_698_server_LINK_set_10_fcs(void** state);
//11、无结束字符测试
void test_app_698_server_LINK_set_11_end(void** state);


///////////////////////////////////////////////////////////////
//控制字测试用例
//1、传输方向位DIR与启动标志位PRM测试
void test_app_698_server_LINK_ctrl_dir_prm(void** state);
//2、分帧标志测试
void test_app_698_server_LINK_ctrl_frm(void** state);
//3、分帧传输其他测试项
void test_app_698_server_LINK_ctrl_frm_other(void** state);
//4、扰码标志位测试
void test_app_698_server_LINK_ctrl_sc(void** state);

#endif //__test_app_698_server_uniformity_H__