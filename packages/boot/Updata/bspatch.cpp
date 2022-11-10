#include <string.h>
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>

//static off_t unzip_data_pos;
//static off_t unzip_data_maxlen;
static structpatch_data handle_data;
static Unzip_Ctrl_Data Patch_Unzip_Ctrl_Data;
static Patch_Status handle_data_status;
static int data_counter = 0;




int Patchdata_handler(structpatch_data handle_data, FILE_ID *old_file, FILE_ID *new_file){
    uint8_t read_data;
    uint8_t write_data;
    int i;
    if(handle_data.patchctrl_data.patchdata1_len){
        if(handle_data.patchctrl_data.patchdata1_pos != old_file->offset){
            file_lseek(old_file, handle_data.patchctrl_data.patchdata1_pos);
        }
        for(i = 0; i < handle_data.patchctrl_data.patchdata1_len; i++){
            if(file_read(old_file, &read_data, 1) != 1)
                return -1;
            write_data = read_data + handle_data.patchdata[i];
            if(file_write(new_file, &write_data, 1) != 1)
                return -1;
        }

    }
    if(handle_data.patchctrl_data.patchdata2_len){
        if(file_write(new_file, &(handle_data.patchdata[handle_data.patchctrl_data.patchdata1_len]),  \
                      handle_data.patchctrl_data.patchdata2_len) != handle_data.patchctrl_data.patchdata2_len)
            return -1;
    }
    return 0;
}


void Handle_Data_Init(off_t data){
    //unzip_data_pos = 0;
    data_counter = 0;
    //unzip_data_maxlen = data;
    memset(&handle_data, 0, sizeof(structpatch_data));
    memset(&Patch_Unzip_Ctrl_Data, 0, sizeof(Unzip_Ctrl_Data));
    handle_data_status = Handle_Ctrl_Ctrl_Byte;
}

int Unzipdata_Handle(uint8_t data, FILE_ID *old_file, FILE_ID *new_file){
    static int ctrl_size = 0;
    static int diff_size = 0;
    static int extra_size = 0;
    switch(handle_data_status){
    case Handle_Ctrl_Ctrl_Byte:
        ctrl_size = (data >> 6) + 1;
        diff_size = (data >> 3) & 0x07;
        extra_size = data & 0x07;
        handle_data_status = Handle_Ctrl_Index_Byte;
        data_counter = 0;
        break;
    case Handle_Ctrl_Index_Byte:
        switch(data_counter){
        case 0:
            Patch_Unzip_Ctrl_Data.Olddata_Index = data;
            data_counter = 1;
            break;
        case 1:
            Patch_Unzip_Ctrl_Data.Olddata_Index += data << 8;
            data_counter = 2;
            break;
        case 2:
            Patch_Unzip_Ctrl_Data.Olddata_Index += data << 16;
            data_counter = 3;
            break;
        case 3:
            Patch_Unzip_Ctrl_Data.Olddata_Index += data << 24;
            data_counter = 4;
            break;
        default:
            return -1;
        }
        if(data_counter == ctrl_size){
            handle_data.patchctrl_data.patchdata1_pos = Patch_Unzip_Ctrl_Data.Olddata_Index;
            handle_data.patchctrl_data.patchdata1_len = 0;
            handle_data.patchctrl_data.patchdata2_len = 0;

            if(diff_size){
                handle_data_status = Handle_Ctrl_DiffLen_Byte;
            }
            else if(extra_size){
                handle_data_status = Handle_Ctrl_ExtraLen_Byte;
            }
            else{
                handle_data_status = Handle_Ctrl_Ctrl_Byte;
            }
            data_counter = 0;
        }
        break;
    case Handle_Ctrl_DiffLen_Byte:
        switch(data_counter){
        case 0:
            Patch_Unzip_Ctrl_Data.Diffdata_Len = data;
            data_counter = 1;
            break;
        case 1:
            Patch_Unzip_Ctrl_Data.Diffdata_Len += data << 8;
            data_counter = 2;
            break;
        case 2:
            Patch_Unzip_Ctrl_Data.Diffdata_Len += data << 16;
            data_counter = 3;
            break;
        case 3:
            Patch_Unzip_Ctrl_Data.Diffdata_Len += data << 24;
            data_counter = 4;
            break;
        default:
            return -1;
        }
        if(data_counter == diff_size){
            if(extra_size){
                handle_data_status = Handle_Ctrl_ExtraLen_Byte;
            }
            else{
                handle_data_status = Handle_Unzip_Data;
            }
            data_counter = 0;
        }
        break;
    case Handle_Ctrl_ExtraLen_Byte:
        switch(data_counter){
        case 0:
            Patch_Unzip_Ctrl_Data.Extradata_Len = data;
            data_counter = 1;
            break;
        case 1:
            Patch_Unzip_Ctrl_Data.Extradata_Len += data << 8;
            data_counter = 2;
            break;
        case 2:
            Patch_Unzip_Ctrl_Data.Extradata_Len += data << 16;
            data_counter = 3;
            break;
        case 3:
            Patch_Unzip_Ctrl_Data.Extradata_Len += data << 24;
            data_counter = 4;
            break;
        default:
            return -1;
        }
        if(data_counter == extra_size){
            handle_data_status = Handle_Unzip_Data;
            data_counter = 0;
        }
        break;
    case Handle_Unzip_Data:
        if(Patch_Unzip_Ctrl_Data.Diffdata_Len > 0){
            Patch_Unzip_Ctrl_Data.Diffdata_Len--;
            handle_data.patchctrl_data.patchdata1_len++;
            handle_data.patchdata[data_counter++] = data;
        }
        else if(Patch_Unzip_Ctrl_Data.Extradata_Len > 0){
            Patch_Unzip_Ctrl_Data.Extradata_Len--;
            handle_data.patchctrl_data.patchdata2_len++;
            handle_data.patchdata[data_counter++] = data;
        }

        if((Patch_Unzip_Ctrl_Data.Diffdata_Len <= 0) && (Patch_Unzip_Ctrl_Data.Extradata_Len <= 0)){
            handle_data_status = Handle_Ctrl_Ctrl_Byte;
            if(Patchdata_handler(handle_data, old_file, new_file) == -1){
                return -1;
            }
            handle_data.patchctrl_data.patchdata1_pos += handle_data.patchctrl_data.patchdata1_len;
            handle_data.patchctrl_data.patchdata1_len = 0;
            handle_data.patchctrl_data.patchdata2_len = 0;
            data_counter = 0;
        }
        if(data_counter == PATCHDATA_LEN){
            if(Patchdata_handler(handle_data, old_file, new_file) == -1){
                return -1;
            }
            handle_data.patchctrl_data.patchdata1_pos += handle_data.patchctrl_data.patchdata1_len;
            handle_data.patchctrl_data.patchdata1_len = 0;
            handle_data.patchctrl_data.patchdata2_len = 0;
            data_counter = 0;
        }

        break;
    default:
        break;
    }
    return 0;
}



uint8_t data[4096] = {0};

int BsPatch(FILE_ID *old_file, FILE_ID *patch_file, FILE_ID *new_file)
{
    uint8_t *data_buff;
    uint8_t zip_flag;
    uint8_t read_data;
    uint8_t out_data;
    off_t databuff_len;
    off_t databuff_pos;
    int index_pos;
    int match_len;
    uint8_t matchlen_compen;
    int Index_Mask;
    int Match_Mask;
    int ctrl_data;
    int Singledata_Len;
    uint8_t right_len;

    file_lseek(patch_file, 4);
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = read_data;
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = ctrl_data | read_data << 8;
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = ctrl_data | read_data << 16;
    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    ctrl_data = ctrl_data | read_data << 24;
    Handle_Data_Init(ctrl_data);

    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    zip_flag = read_data;
    databuff_len = 1 << (10 + zip_flag);
    if(databuff_len != 4096)
      return -1;
    
    data_buff = data;//new uint8_t[databuff_len];


    databuff_pos = 0;

    if(file_read(patch_file, &read_data, 1) != 1){
        return -1;
    }
    data_buff[databuff_pos++] = out_data = read_data;
    if(Unzipdata_Handle(out_data, old_file, new_file) == -1){
        return -1;
    }
    databuff_pos = databuff_pos % databuff_len;

    while(file_read(patch_file, &read_data, 1) == 1){

        ctrl_data = read_data;
        if(ctrl_data >> 4 == 0x0f){
            Singledata_Len = (ctrl_data & 0x0f) + 1;
            for(int i = 0; i < Singledata_Len; i++){
                if(file_read(patch_file, &read_data, 1) != 1){
                    return -1;
                }
                data_buff[databuff_pos++] = out_data = read_data;
                if(Unzipdata_Handle(out_data, old_file, new_file) == -1){
                    return -1;
                }
                databuff_pos = databuff_pos % databuff_len;
            }
            continue;
        }
         else if(ctrl_data >> 7 == 0){
            right_len = 1;
            matchlen_compen = 2;
            Index_Mask = 0x3F << 1;
            Match_Mask = 0x1;
        }
        else if(ctrl_data >> 6 == 2){
            if(file_read(patch_file, &read_data, 1) != 1){
                return -1;
            }
            ctrl_data = ((ctrl_data & 0x3f) << 8) | read_data;
            right_len = 4 - (int)(zip_flag / 2);
            matchlen_compen = 3;
            Index_Mask = ((1 << (10 + (int)(zip_flag / 2))) - 1) << (4 - (int)(zip_flag / 2));
            Match_Mask = (1 << (4 - (int)(zip_flag / 2))) - 1;
        }
        else if(ctrl_data >> 5 == 6){
            if(file_read(patch_file, &read_data, 1) != 1){
                return -1;
            }
            ctrl_data = ((ctrl_data & 0x1f) << 16) | (read_data << 8);
            if(file_read(patch_file, &read_data, 1) != 1){
                return -1;
            }
            ctrl_data = ctrl_data | read_data;
            right_len = 10 - zip_flag;
            matchlen_compen = 4;
            Index_Mask = ((1 << (10 + zip_flag)) - 1) << (10 - zip_flag);
            Match_Mask = (1 << (10 - zip_flag)) - 1;
        }
        else{
            if(file_read(patch_file, &read_data, 1) != 1){
                return -1;
            }
            ctrl_data = ((ctrl_data & 0xf) << 24) | (read_data << 16);
            if(file_read(patch_file, &read_data, 1) != 1){
                return -1;
            }
            ctrl_data = ctrl_data | (read_data << 8);
            if(file_read(patch_file, &read_data, 1) != 1){
                return -1;
            }
            ctrl_data = ctrl_data | read_data;
            right_len = 18 - zip_flag;
            matchlen_compen = 5;
            Index_Mask = ((1 << (10 + zip_flag)) - 1) << (18 - zip_flag);
            Match_Mask = (1 << (18 - zip_flag)) - 1;
        }

        index_pos = (ctrl_data & Index_Mask) >> right_len;
        match_len = (ctrl_data & Match_Mask) + matchlen_compen;
        index_pos = (databuff_pos + databuff_len - index_pos - 1) % databuff_len;
        for (int i = 0; i < match_len; i++){
            data_buff[databuff_pos++] = out_data = data_buff[index_pos++];
            if(Unzipdata_Handle(out_data, old_file, new_file) == -1){
                return -1;
            }
            index_pos = index_pos % databuff_len;
            databuff_pos = databuff_pos % databuff_len;
        }
    }

    //delete []data_buff;
    return 1;
}










