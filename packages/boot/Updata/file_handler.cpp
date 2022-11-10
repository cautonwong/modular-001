#include "file_handler.h"
#include "rom_api.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


FILE_ID* file_open(EEPROM_ADDRESS fileinfo_address, uint8_t mode){
    FILE_INFORMATION file_info;


    if(Get_File_Information(fileinfo_address, (uint8_t *)&file_info, sizeof(FILE_INFORMATION)) == -1){
        return NULL;
    }
    if(file_info.file_status == FILE_OPEN){
        return NULL;
    }
    if(mode == 0)
        return NULL;
    FILE_ID *file = (FILE_ID *)malloc(sizeof(FILE_ID));
    if(file == NULL)
        return NULL;
    file->file_address = file_info.file_address;
    file->fileinfo_address = fileinfo_address;
    file->len = file_info.len;
    file->offset = 0;
    file->mode = mode;
    file->buff_change = 0;
    file->buff_pos = file->file_address - ((file->file_address) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
    file->data_buff = (uint8_t *)malloc(FLASH_PAGE_SIZE);
    if(File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                      file->data_buff,     \
                      FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
        free(file->data_buff);
        free(file);
        return NULL;
    }
    file_info.file_status = FILE_OPEN;
    return file;
}

int file_flush(FILE_ID *file){
    if(file->buff_change){
        if(File_Data_Write((file->file_address + file->offset - 1) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                       file->data_buff, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE)
            return -1;
        file->buff_change = 0;
    }
    return 0;
}

int file_lseek(FILE_ID *file, off_t offset){

    if(file->offset == offset)
        return file->len - file->offset;

    if(((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1))) == \
            ((file->file_address + offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)))){
        if(file->mode & FILEWRITE_ONLY){
            file->offset = offset;
            if(file->len < file->offset){
                file->len = file->offset;
            }
        }
        if(file->len < file->offset){
            return -1;
        }
        file->offset = offset;
        file->buff_pos = (file->file_address + file->offset) -   \
                ((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
    }
    else{
        if(file_flush((file)) == -1)
            return -1;
        if(file->mode & FILEWRITE_ONLY){
            file->buff_change = 0;
            if(File_Data_Read((file->file_address + offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                              file->data_buff, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
                File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                              file->data_buff, FLASH_PAGE_SIZE);
                return -1;
            }
            else{
                file->offset = offset;
                file->buff_pos = (file->file_address + file->offset) -  \
                        ((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
                if(file->offset > file->len)
                    file->len = file->offset;
            }
        }
        if(file->mode & FILEREAD_ONLY){
            if(file->offset != offset){
                if(offset > file->len){
                    return -1;
                }
                else if(offset == file->len){
                    file->offset = offset;
                }
                else{
                    if(File_Data_Read((file->file_address + offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                      file->data_buff,     \
                                      FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
                        File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                                      file->data_buff,     \
                                                      FLASH_PAGE_SIZE);
                        return -1;
                    }
                    else{
                        file->offset = offset;
                        file->buff_pos = (file->file_address + file->offset) -  \
                                ((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)));
                    }
                }
            }
        }
    }
    return (file->len - file->offset);
}

int file_read(FILE_ID *file, uint8_t *data, off_t len){
    int real_len = 0;
    int buff_len;

    if((!(file->mode & FILEREAD_ONLY)) || file->offset >= file->len)
        return -1;
    len = (len > (file->len - file->offset)) ? (file->len - file->offset) : len;
    while(len > 0){
        buff_len = (len > (FLASH_PAGE_SIZE - file->buff_pos)) ?   \
                    (FLASH_PAGE_SIZE - file->buff_pos) : len;
        memcpy(data + real_len, &file->data_buff[file->buff_pos], buff_len);
        file->buff_pos += buff_len;
        file->offset += buff_len;
        real_len += buff_len;
        len -= buff_len;
        if(file->buff_pos >= FLASH_PAGE_SIZE){
            if(file_flush(file) == -1)
                return -1;
            if((file->offset < file->len) || (file->mode & FILEWRITE_ONLY)){
                if(File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                  file->data_buff,     \
                                  FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
                    file->buff_pos -= buff_len;
                    file->offset -= buff_len;
                    File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                                  file->data_buff, FLASH_PAGE_SIZE);
                    return -1;
                }
                file->buff_pos = 0;
            }

        }
    }
    return real_len;
}

int file_write(FILE_ID *file, uint8_t *data, off_t len){
    int real_len = 0;
    int buff_len;
    if(!(file->mode & FILEWRITE_ONLY))
        return -1;
    while(len > 0){
        buff_len = (len > (FLASH_PAGE_SIZE - file->buff_pos)) ?   \
                    (FLASH_PAGE_SIZE - file->buff_pos) : len;
        memcpy(&file->data_buff[file->buff_pos], data + real_len, buff_len);
        file->buff_pos += buff_len;
        file->offset += buff_len;
        real_len += buff_len;
        len -= buff_len;
        file->buff_change = 1;
        if(file->offset > file->len)
            file->len = file->offset;
        if(file->buff_pos >= FLASH_PAGE_SIZE){
            if(file_flush(file) == -1)
                return -1;
            if(File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                              file->data_buff, FLASH_PAGE_SIZE) != FLASH_PAGE_SIZE){
                file->buff_pos -= buff_len;
                file->offset -= buff_len;
                File_Data_Read((file->file_address + file->offset) & (~(FLASH_ADDRESS)(FLASH_PAGE_SIZE - 1)),   \
                                              file->data_buff, FLASH_PAGE_SIZE);
                return -1;
            }
            file->buff_pos = 0;
        }
    }
    return real_len;
}

int file_close(FILE_ID *file){
    FILE_INFORMATION file_info;

    file_flush(file);
    free(file->data_buff);
    file_info.file_address = file->file_address;
    file_info.len = file->len;
    file_info.file_status = FILE_CLOSE;
    if(Save_File_Information(file->fileinfo_address, (uint8_t *)&file_info, sizeof(FILE_INFORMATION)) == -1){
        return -1;
    }
    free(file);
    return 0;
}
