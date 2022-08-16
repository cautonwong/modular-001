
#define	Mem_RAM				1
#define	Mem_FLASH			2
#define	Mem_E2PROM_BP		4
#define	Mem_E2PROM			8
#define MEMVALUE            0x0F

//´íÎóĞÅÏ¢×Ö¶¨Òå
#define 	RS_ERR_LEN		        	0x01//Êı¾İÓò³¤¶È´íÎó,ÆäËü´íÎó
#define 	RS_ERR_OTHERS		       	0x01//ÆäËü´íÎó
#define 	RS_ERR_ID		        	0x02//ID´íÎó
#define 	RS_ERR_PASS		        	0x04//ÃÜÂë´í/Î´ÊÚÈ¨
#define 	RS_ERR_WR		        	0x04//¶ÁĞ´È¨ÏŞ
#define 	RS_ERR_BAUT		        	0x08//²¨ÌØÂÊ²»ÄÜ¸ü¸Ä
#define     RS_ERR_SEAOV                0x10//ÄêÊ±ÇøÊı³¬
#define     RS_ERR_TOUOV                0x20//ÈÕÊ±¶ÎÊı³¬
#define     RS_ERR_FEEOV                0x40//·ÑÂÊÊı³¬

//Info32bit¶¨Òå
// 6 bits	Êı¾İ³¤¶È
#define		CommDataLen				0x0000003F
// 2 bits	Êı¾İ´æ´¢ÀàĞÍ
#define 	CommData_RAM			0x0040*Mem_RAM      //0x00000040
#define 	CommData_FLASH			0x0040*Mem_FLASH    //0x00000080
#define 	CommData_E2PROM_BP		0x0040*Mem_E2PROM_BP//0x00000100   //e2p å¤‡ä»½åŒº
#define 	CommData_E2PROM			0x0040*Mem_E2PROM   //0x00000200

// 1 bits	Êı¾İ´æ´¢¸ñÊ½×ª»»
#define 	CommData_BCDTOHEX		0x00000400
// 1 bits	Êı¾İÍ¨Ñ¶¸ñÊ½
#define 	CommData_HEX			0x00000800
// 1 bits	Êı¾İ´æ´¢Î»ÖÃ(°´ÀúÊ·¼ÇÂ¼)
#define		CommData_His			0x00001000
// 2 bits	¶ÁĞ´¿ØÖÆ
#define		CommData_R				0x00002000
#define		CommData_W				0x00004000
#define		CommData_WR				0x00006000
// 2 bits	°²È«È¨ÏŞÀàĞÍ
#define		CommData_PROGRAM		0x00010000//±à³Ì×´Ì¬ÏÂ²ÅÔÊĞíÖ´ĞĞ
#define		CommData_FACTORY		0x00020000//¹¤³§Ä£Ê½ÏÂ²ÅÔÊĞíÖ´ĞĞ
#define		CommData_SECONDLY		0x00040000//µÚ¶şÀàÊı¾İ



typedef struct
{
	const unsigned long	ID;//Í¨Ñ¶ID
	unsigned int	Addr;//Êı¾İµØÖ·
	const unsigned long    Info;//Êı¾İĞÅÏ¢
} COMMTAB;

extern const  COMMTAB  CommTab[242];

#define		CommTabNum			sizeof(CommTab)/sizeof(COMMTAB)



