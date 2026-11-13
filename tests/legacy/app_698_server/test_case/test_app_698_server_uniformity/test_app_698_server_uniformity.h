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


//////////////////////////////////////////////////////////////

//建立应用连接测试用例
//1、期望的应用层协议版本号测试
//2、期望的协议一致性块测试
//3、期望的功能一致性块测试
//4、客户机发送帧最大尺寸
//5、客户机接收帧最大尺寸
//7、客户机接收帧最大窗口尺寸
//8、客户机最大可处理APDU尺寸
//9、期望的应用连接超时时间
//10、认证请求对象

void test_app_698_server_CONNECT_Request(void** state);

//1、期望的应用层协议版本号测试
void test_app_698_server_CONNECT_Request_1_version(void** state);

//2、期望的协议一致性块测试
void test_app_698_server_CONNECT_Request_2_pro(void** state);

//3、期望的功能一致性块测试
void test_app_698_server_CONNECT_Request_3_func(void** state);

//4、客户机发送帧最大尺寸
void test_app_698_server_CONNECT_Request_4_tx_max(void** state);

//5、客户机接收帧最大尺寸
void test_app_698_server_CONNECT_Request_5_rx_max(void** state);

//7、客户机接收帧最大窗口尺寸
void test_app_698_server_CONNECT_Request_7_win_max(void** state);

//8、客户机最大可处理APDU尺寸
void test_app_698_server_CONNECT_Request_8_apdu_max(void** state);

//9、期望的应用连接超时时间
void test_app_698_server_CONNECT_Request_9_timeout(void** state);

//10、认证请求对象
void test_app_698_server_CONNECT_Request_10_auth(void** state);



////////////////////////////////////////////////////////////////////////////////////////

//Get-Requeset数据类型测试用例
//1、读取一个对象属性请求Get-Requeset数据类型测试 test_app_698_server_GET_Request_0501
void test_app_698_server_GET_Request_0501(void** state);
//2、读取若干个对象属性请求Get-RequesetList数据类型测试 test_app_698_server_GET_Request_0502
void test_app_698_server_GET_Request_0502(void** state);
//3、读取一个记录型对象属性GetRequestRecord数据类型测试 test_app_698_server_GET_Request_0503
//4、读取若干个记录型对象属性GetRequestRecordList数据类型测试 test_app_698_server_GET_Request_0504
//5、读取分帧响应的下一个数据块请求GetRequestNext数据类型测试 
//6、读取一个对象属的MD5值GetRequestMD5数据类型测试


//Get-Response数据类型测试用例
//1、读取一个对象属性响应Get-Response数据类型测试 test_app_698_server_GET_Response_8501
void test_app_698_server_GET_Response_8501(void** state);
//2、读取若干个对象属性响应Get-ResponseList数据类型测试 test_app_698_server_GET_Response_8502
void test_app_698_server_GET_Response_8502(void** state);
//3、读取一个记录型对象响应GetResponseRecord数据类型测试 test_app_698_server_GET_Response_8503
//4、读取若干个记录型对象属性响应GetResponseRecordList数据类型测试 test_app_698_server_GET_Response_8504
//5、读取分帧响应的下一个数据块响应GetResponseNext数据类型测试  不支持
//6、读取一个对象属的MD5值响应GetResponseMD5数据类型测试


////////////////////////////////////////////////////////////////////////////////////////
//Set-Requeset数据类型测试用例
//1、设置一个对象属性请求Set-Requeset数据类型测试 test_app_698_server_SET_Request_0601
void test_app_698_server_SET_Request_0601(void** state);
//2、设置若干个对象属性请求Set-RequesetList数据类型测试 test_app_698_server_SET_Request_0602
void test_app_698_server_SET_Request_0602(void** state);
//3、设置后读取若干个对象属性SetThenGetRequestNormalList数据类型测试 test_app_698_server_SET_Request_0603
void test_app_698_server_SET_Request_0603(void** state);

//Set-Responset数据类型测试用例
//1、设置一个对象属性响应Set-Response数据类型测试 test_app_698_server_SET_Response_8601
void test_app_698_server_SET_Response_8601(void** state);
//2、设置若干个对象属性响应Set-Response数据类型测试 test_app_698_server_SET_Response_8602
void test_app_698_server_SET_Response_8602(void** state);
//3、设置后读取若干个对象属性响应SetThenGetResponseNormalList数据类型测试 test_app_698_server_SET_Response_8603
void test_app_698_server_SET_Response_8603(void** state);
////////////////////////////////////////////////////////////////////////////////////////


//Action-Request数据类型测试用例
//1、操作一个对象属性请求Action-Request数据类型测试 test_app_698_server_ACTION_Request_0701
void test_app_698_server_ACTION_Request_0701(void** state);
//2、操作若干个对象属性请求Action-RequestNormalList数据类型测试 test_app_698_server_ACTION_Request_0702
void test_app_698_server_ACTION_Request_0702(void** state);
//3、操作后读取若干个对象属性请求Action-ThenGetRequestNormalList数据类型测试 test_app_698_server_ACTION_Request_0703
void test_app_698_server_ACTION_Request_0703(void** state);

//Action-Response数据类型测试用例
//1、操作一个对象属性响应Action-Response数据类型测试 test_app_698_server_ACTION_Response_8701
void test_app_698_server_ACTION_Response_8701(void** state);
//2、操作若干个对象属性响应Action-ResponseNormalList数据类型测试 test_app_698_server_ACTION_Response_8702
void test_app_698_server_ACTION_Response_8702(void** state);
//3、操作后读取若干个对象属性响应Action-ThenGetResponseNormalList数据类型测试 test_app_698_server_ACTION_Response_8703
void test_app_698_server_ACTION_Response_8703(void** state);





#endif //__test_app_698_server_uniformity_H__