#ifndef __UPGRADE_645_97__H_
#define  __UPGRADE_645_97__H_

#include "nidavellir.h"
#include "645id_table.h"

#define UPGRADE_MODE_ONLINE   0 //在线升级
#define UPGRADE_MODE_LOCAL    1 //本地升级 //存储文件到本地

#define UPGRADE_MODE  (UPGRADE_MODE_ONLINE)


#define	IMAGE_MAP_LEN				1024	                    //位图信息长度 
#define	IMAGE_PARA_ID				UPGRADE_PARA_INFO          //升级参数e2p 绝对地址 从0开始             60字节 
#define	IMAGE_PARA_ID2			    UPGRADE_RESET_FLAG         //升级复位标志4字节 
#define	IMAGE_PARA_BLOCK			mbase_block_parameter_1     //参数块

#define	IMAGE_DATA_ID_BLOCK			mbase_block_event_flash1   //flash 首个块 绝对地址从0开始， 和分钟冻结共用              
#define	FILE_TYPE					0xfd
#define	IMAGE_ENABLE_FLAG			0x5A6B7C8D	/*处于升级模式*/


#define UPGRADE_FRZMIN_DLY_SEC      (30*60) // 升级过程中  分钟冻结 禁止执行时间-单相

enum image_transfer_step_t
{
	transfer_not_initiated,

	transfer_initiated,

	transfer_set_mode,

	image_identify,

	transfer_image_data,

	transfer_bit_map,

	verification_initiated,

	verification_successful,

	verification_failed,

	activation_initiated,

	activation_successful,

	activation_failed,
};

enum check_image_data_state_t
{
	check_successs = 0x00,
	check_temporary_failure = 0x01,
	check_other_reason = 0x03,
};

enum image_verify_state_t
{
	image_verification_initiated = 0x02,
	image_verification_successful = 0x03,
	image_verification_failed = 0x04,
};

enum image_transfer_mode_t
{
	mode_01 = 0x01,/*0x01升级模式 在线升级，plc imageBlockSize固定为128字节*/
	mode_02 = 0x02,/*0x02升级模式 在线升级，rs485 imageBlockSize固定为196字节*/ 
	mode_03 = 0x03,/*0x03升级模式     在线升级，IR imageBlockSize固定为128字节*/
	mode_81 = 0x81,/*0x01升级模式 本地升级，plc imageBlockSize固定为128字节*/
	mode_82 = 0x82,/*0x02升级模式 本地升级，rs485 imageBlockSize固定为196字节*/ 
	mode_83 = 0x83,/*0x03升级模式     本地升级，plc imageBlockSize固定为128字节*/
};

struct image_transfer_state_t
{
	uint8_t 	transfer_enable;	/*	0x5a: 使能打开  0x00:使能关闭 						*/
	uint8_t	transfer_mode;		/*	0x01升级模式一为针对载波、RF等通信方式，imageBlockSize
								固定为64字节；0x02升级模式二为针对光电、RS485、GPRS等通
								信方式，imageBlockSize固定为192字节。*/
	uint8_t 	block_size;			/*	block长度					  						*/
	uint8_t 	last_blocksize;		/*	最后一帧长度										*/

	uint8_t 	file_type;			/*	文件类型											*/
	uint8_t 	encryption_type;	/*	加密类型											*/
	uint8_t 	check_type;			/*	校验类型											*/
	uint8_t 	version[20];		/*	版本信息，高字节在后								*/
	uint8_t 	code_crc[16];		/*	程序校验码，高字节在后								*/

	uint8_t 	enable_rst[4];		/*	是否开始升级标志，0x5aa5校验通过开始升级，0000不升级*/
	uint8_t   block_num[4];		/*	block总数											*/

	uint8_t   datatime[4];		/*	激活时间，转换为秒*/
	uint8_t	image_enable[4];	/*处于升级模式*/
};

enum image_transfer_offset_t
{
	transfer_enable_offset = 0x00,

	transfer_mode_offset = 0x01,

	block_size_offset = 0x02,

	last_blocksize_offset = 0x03,

	file_type_offset = 0x04,

	encryption_type_offset = 0x05,



};

struct mode_frame_len_t
{
	uint8_t mode;
	uint8_t frame_len;
};


struct pro_imange_transfer_id_t
{
	uint16_t 	id;
	uint8_t	attr;/*bit0 1:带密码验证，0:无需密码验证	其他:保留，默认为 0*/
	uint8_t 	length;
	uint8_t	(*pro_id)(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
};

struct image_read_id_t
{
	uint16_t dl645_id;
	uint8_t(*pro_down_ops)(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
	uint8_t(*pro_up_ops)(struct instance_t* instance, uint8_t *p_rx, uint8_t *p_tx, uint8_t *tx_len, struct context_645_t* ctx);
};


extern const struct mode_frame_len_t CODE mode_frame[];

extern uint8_t get_imange_transfer_para(struct instance_t* instance, uint8_t *buff, const uint8_t len, const uint8_t offset);

extern uint8_t upgrade_mode_judge(const uint8_t mode);

extern uint8_t set_image_state(struct instance_t* instance, const uint8_t state, struct image_transfer_state_t *image_transfer_state);

extern const struct pro_imange_transfer_id_t * find_image_transfer_process(const uint16_t id);

extern const struct image_read_id_t  *find_image_read_id_processor(uint16_t id);



#endif
