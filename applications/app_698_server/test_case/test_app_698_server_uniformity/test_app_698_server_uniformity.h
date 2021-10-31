#ifndef __test_app_698_server_uniformity_H__
#define __test_app_698_server_uniformity_H__

//数据链路层合法性
//读取一个对象属性（GET）数据链路层测试用例

//1 客户机发起正确GET请求测试 如读取读取安全模式参数
void test_app_698_server_LINK_get_ok(void** state);

//2 错误帧起始符测试
void test_app_698_server_LINK_get_header_err(void** state);

//3 错误长度测试
void test_app_698_server_LINK_get_len_err(void** state);

//4 错误控制域测试
void test_app_698_server_LINK_get_ctrl_err(void** state);


//5 错误通信地址测试
//6 错误客户机地址测试
//7 错误逻辑地址测试
//8 错误HCS帧头校验测试
//9 错误GET-Request测试
//10 错误属性测试
//11 错误FCS帧校验测试
//12 结束字符测试


///////////////////////////////////////////////////////////////////////
//设置一个对象属性（SET）数据链路层测试用例
//1、客户机发起正确SET请求测试
//2、错误帧起始符测试
//3、错误长度测试
//4、错误控制域测试
//5、错误通信地址测试
//6、错误客户机地址测试
//7、错误逻辑地址测试
//8、错误HCS帧头校验测试
//9、错误SET-Request测试
//10、错误FCS帧校验测试
//11、无结束字符测试
void test_app_698_server_LINK_set(void** state);

//控制字测试用例
//1、传输方向位DIR与启动标志位PRM测试
//2、分帧标志测试
//3、分帧传输其他测试项
//4、扰码标志位测试
void test_app_698_server_LINK_ctrl(void** state);

#endif //__test_app_698_server_uniformity_H__