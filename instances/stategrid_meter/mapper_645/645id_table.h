
#define	Mem_RAM				1
#define	Mem_FLASH			2
#define	Mem_E2PROM			3
#define MEMVALUE            0x03

//Info16bit定义
// 6 bits	数据长度
#define		CommDataLen				0x003F
// 2 bits	数据存储类型
#define 	CommData_RAM			0x0040*Mem_RAM
#define 	CommData_FLASH			0x0040*Mem_FLASH
#define 	CommData_E2PROM			0x0040*Mem_E2PROM

// 1 bits	数据存储格式转换
#define 	CommData_BCDTOHEX		0x0200
// 1 bits	数据通讯格式
#define 	CommData_HEX			0x0400
// 1 bits	数据存储位置(按历史记录)
#define		CommData_His			0x0800
// 2 bits	读写控制
#define		CommData_R				0x1000
#define		CommData_W				0x2000
#define		CommData_WR				0x3000
// 2 bits	安全权限类型
#define		CommData_PROGRAM		0x4000//编程状态下才允许执行
#define		CommData_FACTORY		0x8000//工厂模式下才允许执行
#define		CommData_SECONDLY		0xC000//第二类数据

typedef struct
{
	const unsigned long	ID;//通讯ID
	unsigned int	Addr;//数据地址
	const unsigned int    Info;//数据信息
} COMMTAB;

extern const  COMMTAB  CommTab[242];

#define		CommTabNum			sizeof(CommTab)/sizeof(COMMTAB)



