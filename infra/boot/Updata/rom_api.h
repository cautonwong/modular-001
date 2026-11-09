#ifndef ROM_API_H
#define ROM_API_H

typedef unsigned char uint8_t;
typedef long    off_t;
typedef unsigned int FLASH_ADDRESS;
#define EEPROM_ADDRESS FLASH_ADDRESS


#define FLASH_PAGE_SIZE 256


typedef struct{
    FLASH_ADDRESS file_address;
    EEPROM_ADDRESS fileinfo_address;
    uint8_t mode;
    off_t len;
    off_t offset;
    uint8_t *data_buff;
    uint8_t buff_change;
    off_t buff_pos;
} FILE_ID;



typedef struct{
    FLASH_ADDRESS file_address;
    off_t len;
    uint8_t file_status;
} FILE_INFORMATION;






/*************************************************
Function: int eeprom_read(FLASH_ADDRESS address, uint8_t *data, int data_size)
Description: 从address位置读取信息，address并不一定是页面对齐的，这个函数具体内容需要
    用户实现。
Input:
    address：存储数据的地址，并不一定页对齐
    data: 读取数据后放置的地址
    dataz_size: 读取数据的大小
Output:
Return:
    success: 返回 非-1的数
    fail: 返回 -1
Others: 被file_open使用，来读取文件信息，即FILE_INFORMATION结构体信息，
    file_open在调用该函数处理FILE_INFORMATION结构体数据时时并没有使address
    页面对齐，所以尽量使用eeprom或rom存储数据，也可以存储在flash中，这个
    要移植的人自己斟酌使用，并且也可作为读取数据的通用接口。
*************************************************/
int eeprom_read(FLASH_ADDRESS address, uint8_t *data, int data_size);

/*************************************************
Function: int eeprom_write(FLASH_ADDRESS address, uint8_t *data, int data_size)
Description: 向address位置写入信息，address并不一定是页面对齐的，这个函数具体内容需要
    用户实现。
Input:
    address：存储数据的地址，并不一定页对齐
    data: 要写入的数据的地址
    dataz_size: 写入数据的大小
Output:
Return:
    success: 返回 非-1的数
    fail: 返回 -1
Others: 被file_close使用，来存储文件信息，即FILE_INFORMATION结构体信息，
    file_close在调用该函数时并没有使address页面对齐，所以尽量使用eeprom或rom存
    储数据，也可以存储在flash中，这个要移植的人自己斟酌使用。
*************************************************/
int eeprom_write(FLASH_ADDRESS address, uint8_t *data, int data_size);


/*************************************************
Function: int flash_read(FLASH_ADDRESS address, uint8_t *data, int data_size)
Description: 从address位置读取数据，file读写函数调用时输入address为一定为页面地址
    对齐的，一个页的大小通过FLASH_PAGE_SIZE来定义，这个函数具体内容需要用户实现。
Input:
    address：存储数据的地址，一定页对齐
    data: 读取数据后放置的地址
    dataz_size: 读取数据的大小
Output:
Return:
    success: 返回 非-1的数
    fail: 返回 -1
Others: 被file_flush调用，用于存储文件系统缓冲区的数据，在写入时总是一次一个页面大小，
    并且地址是页对齐的。
*************************************************/
int flash_read(FLASH_ADDRESS address, uint8_t *data, int data_size);


/*************************************************
Function: int flash_write(FLASH_ADDRESS address, uint8_t *data, int data_size)
Description: 向address位置写入数据，file读写函数调用时输入address为一定为页面地址
    对齐的，一个页的大小通过FLASH_PAGE_SIZE来定义，这个函数具体内容需要用户实现。
Input:
    address：存储数据的地址，一定页对齐
    data: 需要写入的数据的放置地址
    dataz_size: 写入数据的大小
Output:
Return:
    success: 返回 非-1的数
    fail: 返回 -1
Others: 被file_read,file_open,file_lseek调用，用于读取文件在flash中的数据，
    在读取时总是一次一个页面大小，且地址是页对齐的。
*************************************************/
int flash_write(FLASH_ADDRESS address, uint8_t *data, int data_size);

#define Get_File_Information(address, data, data_size) eeprom_read((address), (data), (data_size))
#define Save_File_Information(address, data, data_size) eeprom_write((address), (data), (data_size))
#define File_Data_Read(address, data, data_size) flash_read((address), (data), (data_size))
#define File_Data_Write(address, data, data_size) flash_write((address), (data), (data_size))

#endif // ROM_API_H
