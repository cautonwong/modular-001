
#define data_max_len 222


#define     MAX_READ_DATAFILED_LEN     (200)
#define     MAX_WRITE_DATAFILED_LEN    230
#define     APDU_DATA_LEN              (MAX_READ_DATAFILED_LEN - 4)
#define     APDU_PUSH_DATA_LEN         (MAX_READ_DATAFILED_LEN - 4 - 1)

#define     FOLLOW_FRAM_FLAG               0x20
#define     ERR_RESPONSE_FLAG              0x40




#define	Mem_RAM				1
#define	Mem_FLASH			2
#define	Mem_E2PROM_BP		4
#define	Mem_E2PROM			8
#define MEMVALUE            0x0F

//错误信息字定义
#define 	RS_ERR_LEN		        	0x01//数据域长度错误,其它错误
#define 	RS_ERR_OTHERS		       	0x01//其它错误
#define 	RS_ERR_ID		        	0x02//ID错误
#define 	RS_ERR_PASS		        	0x04//密码错/未授权
#define 	RS_ERR_WR		        	0x04//读写权限
#define 	RS_ERR_BAUT		        	0x08//波特率不能更改
#define     RS_ERR_SEAOV                0x10//年时区数超
#define     RS_ERR_TOUOV                0x20//日时段数超
#define     RS_ERR_FEEOV                0x40//费率数超

//新表准远程控制异常应答状态字
#define		ERR_HALT			        0x0001
#define		ERR_ERRMAC			        0x0002
#define		ERR_OTHERR			        0x0004
#define		ERR_AUTHENTM_OVER           0x0008
#define		ERR_CMDTM_INVALID		    0x0010
#define		ERR_RELAYOFF_BAODIAN		0x0020
#define		ERR_RELAYOFF_ALARM_BAODIAN	0x0040
#define		ERR_RELAYOFF_ALARM_INVALID 	0x0080
#define		ERR_RELAYOFF_ALARM_RELAYOFF	0x0100

//新标准远程控制应答状态字
#define		CTRL_STATUE_RELAYON		    0x0001
#define		CTRL_STATUE_RELAYON_EN		0x0002
#define		CTRL_STATUE_RELAYOFF		0x0004
#define		CTRL_STATUE_RELAYOFF_DT		0x0008
#define		CTRL_STATUE_RELAYOFF_ALARM	0x0010
#define		CTRL_STATUE_RELAYOFF_IMAX	0x0020
#define		CTRL_STATUE_BAODIAN_ON		0x0100
#define		CTRL_STATUE_BAODIAN_OFF		0x0200
#define		CTRL_STATUE_ALARM_ON		0x0400
#define		CTRL_STATUE_ALARM_OFF		0x0800





//Info32bit定义
// 6 bits	数据长度
#define		CommDataLen				0x0000003F
// 2 bits	数据存储类型
#define 	CommData_RAM			0x0040*Mem_RAM      //0x00000040
#define 	CommData_FLASH			0x0040*Mem_FLASH    //0x00000080
#define 	CommData_E2PROM_BP		0x0040*Mem_E2PROM_BP//0x00000100   //e2p backup
#define 	CommData_E2PROM			0x0040*Mem_E2PROM   //0x00000200

// 1 bits	数据存储格式转换
#define 	CommData_BCDTOHEX		0x00000400
// 1 bits	数据通讯格式
#define 	CommData_HEX			0x00000800
// 1 bits	数据存储位置(按历史记录)
#define		CommData_His			0x00001000
// 2 bits	读写控制
#define		CommData_R				0x00002000
#define		CommData_W				0x00004000
#define		CommData_WR				0x00006000
// 2 bits	安全权限类型
#define		CommData_PROGRAM		0x00010000//编程状态下才允许执行
#define		CommData_FACTORY		0x00020000//工厂模式下才允许执行
#define		CommData_SECONDLY		0x00040000//第二类数据


#define     FORMAT_00        0x00    //无特殊处理
#define     FORMAT_01        0x01    
#define     FORMAT_02        0x02    
#define     FORMAT_03        0x03    
#define     FORMAT_04        0x04    
#define     FORMAT_05        0x05    
#define     FORMAT_06        0x06    
#define     FORMAT_07        0x07    
#define     FORMAT_08        0x08    
#define     FORMAT_09        0x09    
#define     FORMAT_0A        0x0A    
#define     FORMAT_0B        0x0B    
#define     FORMAT_0C        0x0C    
#define     FORMAT_0D        0x0D    
#define     FORMAT_0E        0x0E    
#define     FORMAT_0F        0x0F   //645复位主动上报状态字   
#define     FORMAT_10        0x10   //645读取主动上报状态字   

typedef struct
{
	const unsigned long	ID;//通讯ID
	unsigned int	Addr;//数据地址
    unsigned short format;   //格式
	const unsigned long    Info;//数据信息
}COMMTAB;

extern const  COMMTAB  CommTab[242];

#define		CommTabNum			sizeof(CommTab)/sizeof(COMMTAB)


enum meter_address_type645
{

    SA_TYPE_SINGLE645      = 0,    //0-单地址
    SA_TYPE_GENERAL645     = 1,    //1-通配地址
    SA_TYPE_BROADECAST645  = 2,    //2-广播99
};

union dl645_DI_t
{
    struct 
    {
        uint8_t  di0; 
        uint8_t  di1; 
        uint8_t  di2;
        uint8_t  di3;
    }elem;

    uint32_t di;
};


extern int read_00( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx);
extern int read_02( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx);
extern int read_04( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx);
extern int read_06( struct instance_t* instance, unsigned long ID , uint8_t *p_rx, uint8_t* p_tx, uint8_t *tx_len, struct context_645_t* ctx);
extern int read_record_data_form_698_protocol(struct instance_t* instance, uint8_t* buf, uint32_t Record_oad, uint32_t capture_oad, uint8_t last_no, uint16_t bufsize);
extern unsigned char GetCommTable( unsigned long ID );
extern signed char ctl_freeze(struct instance_t* instance, uint8_t chl, uint8_t* p_rx);
extern signed char chg_baut(struct instance_t* instance, uint8_t* p_rx);
extern signed char CommRemoteCtl_Process(struct instance_t* instance,  uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx,  uint16_t RemotCtrResult);
extern int upgrade_file_handle(struct instance_t* instance, uint8_t *data, uint16_t len, uint8_t *data_tx, uint8_t *tx_len);
extern signed char ft_adj_get_meter(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);
extern unsigned char Addr_9_9( unsigned char* Addr );
extern unsigned long GetTotalSec( unsigned char* Addr );
extern unsigned char Addr_A_A( unsigned char* Addr ) ;
extern unsigned char Cmp_MeterAddr( struct instance_t* instance, unsigned char* Addr, unsigned char mode );
extern unsigned char ChangeTime_Format_Check( unsigned char* Addr );
extern unsigned char Cmp_Password(struct instance_t* instance, unsigned char* Addr );
extern signed char ctl_freeze(struct instance_t* instance, uint8_t chl, uint8_t* p_rx);
extern signed char chg_baut(struct instance_t* instance, uint8_t* p_rx);
extern signed char ft_adj_get_meter(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *len);
extern int upgrade_file_handle(struct instance_t* instance, uint8_t *data, uint16_t len, uint8_t *data_tx, uint8_t *tx_len);
extern unsigned char Comm485Baut_Check( unsigned char Baut );
extern int response_x_byte_err_645(uint8_t *p_tx, uint16_t err, uint8_t errLen, uint8_t *tx_len);
extern uint8_t Search_DL645_Prepay_IDTab(struct instance_t* instance, uint32_t id,uint8_t cmd);