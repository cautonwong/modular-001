#include "nidavellir.h"
#include "u_test.h"

#define U_TEST_JSON


#ifdef U_TEST_JSON
#include "cjson_lib.h"
#endif






int hex_to_bin(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return ch - '0';
    ch = tolower(ch);
    if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    return -1;
}

void deblank(char string[]) {
    char* p = string;
    char* q = string;
    int same_space = 0;      //记录相邻的空格

    while (*p != '\0') {
        /*判断是否存在空格*/
        if (*p == ' ') {
            /*进行清除多余的空格操作*/
            p++;
            continue;
        }
        else {
            same_space = 0;
        }
        *q++ = *p++;
    }
    *q = '\0';
}


int hex2bin(char* dst, char* src, uint16_t count)
{
    deblank(src);
    while (count--) {
        int hi = hex_to_bin(*src++);
        int lo = hex_to_bin(*src++);
        if ((hi < 0) || (lo < 0))
            return -1;
        *dst++ = (hi << 4) | lo;
    }
    return 0;
}

#ifdef U_TEST_JSON

struct test_info_t
{
    char test_file[_MAX_FNAME];
    struct cJSON* root;
    int name_len;
};

static struct test_info_t  ut_test;

static struct cJSON* get_json_root(const char* path)
{
    uint32_t length = 0;
    char* data = NULL;
    FILE* fd = NULL;
    struct cJSON* root = NULL;
    struct cJSON* child = NULL;

    /// 打开文件，"rb"：访问方式为打开二进制模式，打开成功的返回值为0
    if (0 != fopen_s(&fd, path, "rb"))
    {
        printf("Open json file failed:\"%s\"\n", path);
        return NULL;
    }

    fseek(fd, 0, SEEK_END);        ///< 设置文件操作指针fd的位置。指向以SEEK_END为基准，偏移0个字节位置
    length = ftell(fd);            ///< 文件位置指针当前位置相对于文件首的偏移字节数
    fseek(fd, 0, SEEK_SET);        ///< 设置文件操作指针fd的位置。指向以SEEK_SET为基准，偏移0个字节位置
    data = (char*)malloc(length);  ///< 申请内存
    fread(data, 1, length, fd);    ///< 从输入流fd中读取最多length个字节到数组指针data中
    fclose(fd);
    printf("Get json file:%s. length:%d.\n", path, length);

    root = cJSON_Parse(data);
    if (root != NULL)
    {
        child = cJSON_GetObjectItem(root, "name");
        if (child != NULL)
        {
            printf("json name:%s\n", child->valuestring);
            char* name = malloc(strlen(path) + 1);
            name = strcpy(name, path);
            child->valuestring = name;
        }
    }
    return root; ///< 将data数据解析成json结构体返回
}


int init_test_suite(const char *path)
{
    char path_buffer[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    
    _splitpath(path, drive, dir, fname, ext);


    struct cJSON* root = NULL;
    _makepath(path_buffer, drive, dir, fname, "json");

    root = get_json_root(path_buffer);
    if (root != NULL)
    {
        ut_test.root = root;
        ut_test.name_len = strlen(fname);
        memcpy(ut_test.test_file, fname, ut_test.name_len);
        return 1;
    }
    return 0;
}


int get_test_suites_num(char* u_test_file)
{
    init_test_suite(u_test_file);
    if (NULL != strstr(u_test_file, ut_test.test_file))
    {
        struct cJSON* root = ut_test.root;
        struct cJSON* child = cJSON_GetObjectItem(root, "suites");
        if (child != NULL)
        {
            return cJSON_GetArraySize(child);
        }
    }
    return 0;
}

int get_test_suite_frame(char* u_test_file, uint8_t* data, uint16_t index, uint8_t type)
{
    if (NULL != strstr(u_test_file, ut_test.test_file))
    {
        struct cJSON* root = ut_test.root;
        struct cJSON* child = NULL;
        struct cJSON* child_next = NULL;

        child = cJSON_GetObjectItem(root, "suites");
        if (child != NULL)
        {
            child_next = cJSON_GetArrayItem(child, index);
            if (child_next != NULL)
            {
                if (TEST_source == type)
                    child = cJSON_GetObjectItem(child_next, "test_source");
                if (TEST_expectation == type)
                    child = cJSON_GetObjectItem(child_next, "test_expectation");

                child = cJSON_GetObjectItem(child, "frame");
                if((child != NULL) && (strlen(child->valuestring)))
                { 
                    int len = strlen(child->valuestring) / 2;
                    hex2bin(data, child->valuestring, len);
                    return len;
                }
                return 0;
            }
        }
    }
    return 0;
}

int get_test_suite_cmd(char* u_test_file, uint8_t* data, uint16_t index, uint8_t type)
{
    if (NULL != strstr(u_test_file, ut_test.test_file))
    {
        struct cJSON* root = ut_test.root;
        struct cJSON* child = NULL;
        struct cJSON* child_next = NULL;

        child = cJSON_GetObjectItem(root, "suites");
        if (child != NULL)
        {
            child_next = cJSON_GetArrayItem(child, index);
            if (child_next != NULL)
            {
                if (TEST_source == type)
                    child = cJSON_GetObjectItem(child_next, "test_source");
                if (TEST_expectation == type)
                    child = cJSON_GetObjectItem(child_next, "test_expectation");

                child = cJSON_GetObjectItem(child, "cmd");
                if ((child != NULL) && (strlen(child->valuestring)))
                {
                    if (strcmp(child->valuestring, "frame_check"))
                        return TEST_frame_check;
                    if (strcmp(child->valuestring, "frame_check_no"))
                        return TEST_frame_check_no;
                    return 0;
                }
                return 0;
            }
        }
    }
    return 0;
}

int get_test_suite_apdu(char* u_test_file, uint8_t* data, uint8_t type, uint16_t index)
{
    if (NULL != strstr(u_test_file, ut_test.test_file))
    {
        struct cJSON* root = ut_test.root;
        struct cJSON* child = NULL;
        struct cJSON* child_next = NULL;

        child = cJSON_GetObjectItem(root, "suites");
        if (child != NULL)
        {
            child_next = cJSON_GetArrayItem(child, index);
            if (child_next != NULL)
            {
                if (TEST_source == type)
                    child = cJSON_GetObjectItem(child_next, "test_source");
                if (TEST_expectation == type)
                    child = cJSON_GetObjectItem(child_next, "test_expectation");

                child = cJSON_GetObjectItem(child, "apdu");
                if ((child != NULL) && (strlen(child->valuestring)))
                {
                    int len = strlen(child->valuestring) / 2;
                    hex2bin(data, child->valuestring, len);
                    return len;
                }
                return 0;
            }
        }
    }
    return 0;
}

int get_test_suite_auth(char* u_test_file, uint8_t* data, uint16_t index, uint8_t type)
{
    if (NULL != strstr(u_test_file, ut_test.test_file))
    {
        struct cJSON* root = ut_test.root;
        struct cJSON* child = NULL;
        struct cJSON* child_next = NULL;

        child = cJSON_GetObjectItem(root, "suites");
        if (child != NULL)
        {
            child_next = cJSON_GetArrayItem(child, index);
            if (child_next != NULL)
            {
                if (TEST_source == type)
                    child = cJSON_GetObjectItem(child_next, "test_source");
                if (TEST_expectation == type)
                    child = cJSON_GetObjectItem(child_next, "test_expectation");

                child = cJSON_GetObjectItem(child, "auth");
                if ((child != NULL) && (strlen(child->valuestring)))
                {
                    if (strcmp(child->valuestring, "auth"))
                        return TEST_auth;
                    if (strcmp(child->valuestring, "auth_no"))
                        return TEST_auth_no;
                    return 0;
                }
                return 0;
            }
        }
    }
    return 0;
}
#endif