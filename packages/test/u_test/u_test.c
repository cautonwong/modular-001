#include "nidavellir.h"
#include "u_test.h"

#define U_TEST_JSON

#ifdef U_TEST_JSON
struct test_info_t
{
    char test_file[_MAX_FNAME];
    mxml_node_t* xml_root;
    int name_len;
};

static struct test_info_t  _test_info;

int init_test_suite(const char *path)
{
    char path_buffer[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    

    _splitpath(path, drive, dir, fname, ext);
    _makepath(path_buffer, drive, dir, fname, "yaml");
    //test(path_buffer);


    FILE* fd = fopen(path_buffer, "r");
    if (fd != NULL)
    {
        mxml_node_t* root = mxmlLoadFile(NULL, fd, MXML_NO_CALLBACK);
        fclose(fd);
        /*
         *  DESCEND含义是不查看任何的子节点在XML元素层次中，仅查看同级的伙伴
         *  MXML_DESCEND_FIRST含义是向下搜索到一个节点的第一个匹配子节点，但
         *  DESCEND含义是不查看任何的子节点在XML元素层次中，
         * */
        if (root != NULL)
        {
            _test_info.xml_root = root;
            _test_info.name_len = strlen(fname);
            memcpy(_test_info.test_file, fname, _test_info.name_len);
            return 1;
        }
    }
    return 0;
}


int get_test_suites_num(char* u_test_file)
{
    init_test_suite(u_test_file);
    if (NULL != strstr(u_test_file, _test_info.test_file))
    {
        mxml_node_t* root = _test_info.xml_root;

        mxml_node_t* node = mxmlFindElement(root, root, "suites", NULL, NULL, MXML_DESCEND);
        if (node != NULL)
        {
            uint8_t* data = mxmlElementGetAttr(node, "num");
  
            return data[0] - 0x30;
        }
    }
#if 0
        mxml_node_t* china = mxmlFindElement(root, root, "china", NULL, NULL, MXM)
        while (china)
            {
                mxml_node_t* info = mxmlWalkNext(china, root, MXML_DESCEND);
                while (info)
                {
                    mxml_node_t* node = mxmlWalkNext(info, root, MXML_DESCEND_FIRS
                    printf("context =  %s\n", mxmlGetText(node, 0));
                    node = mxmlWalkNext(node, root, MXML_NO_DESCEND);
                    printf("context =  %s\n", mxmlGetText(node, 0));
                    node = mxmlWalkNext(node, root, MXML_NO_DESCEND);
                    printf("context =  %s\n", mxmlGetText(node, 0));
                    node = mxmlWalkNext(node, root, MXML_NO_DESCEND);
                    printf("context =  %s\n", mxmlGetText(node, 0));
                    info = mxmlFindElement(info, root, "city", NULL, NULL, MXML_NO_DE
                }
                // arg1: 当前节点 arg2: 顶级节点 arg3: 查找的节点名称 arg4: 属性 arg5: value arg6: 查找模式
                china = mxmlFindElement(china, root, "china", NULL, NULL, MXML_NO_DES
            }
    }
#endif
    return 0;
}

int get_test_suite_frame(char* u_test_file, uint8_t* data, uint16_t index, uint8_t type)
{
    
    if (NULL != strstr(u_test_file, _test_info.test_file))
    {
        mxml_node_t* root = _test_info.xml_root;

        mxml_node_t* test_source_node = mxmlFindElement(root, root, "test_source", "name", NULL, MXML_DESCEND);
        if (test_source_node != NULL)
        {
            mxml_node_t* node = mxmlFindElement(test_source_node, root, "frame", NULL, NULL, MXML_DESCEND);
            if (NULL != node)
            {
                printf("mxmlGetText is %s\n", mxmlGetText(node, NULL));
                printf("mxmlElementGetAttr is %s\n", mxmlElementGetAttr(node, "frame"));
            }
            mxml_node_t* info = mxmlWalkNext(test_source_node, root, MXML_DESCEND);
            //hex2bin(frm_buf, frm[i], sizeof(frm[i]) / 2);
            uint8_t * frame = mxmlElementGetAttr(info, "frame");
            //hex2bin(data, frame, strlen(frame) / 2);
            return 1;
        }
    }
    return 0;
}

int get_test_suite_cmd(char* u_test_file, uint8_t type, uint16_t index)
{
    if (NULL != strstr(u_test_file, _test_info.test_file))
    {
        mxml_node_t* root = _test_info.xml_root;

        mxml_node_t* test_source_node = mxmlFindElement(root, root, "test_source", "name", NULL, MXML_DESCEND);
        if (test_source_node != NULL)
        {
            mxml_node_t* node = mxmlFindElement(test_source_node, root, "cmd", NULL, NULL, MXML_DESCEND);
            if (NULL != node)
            {
                printf("mxmlGetText is %s\n", mxmlGetText(node, NULL));

                printf("mxmlElementGetAttr is %s\n", mxmlElementGetAttr(node, "cmd"));

            }
            return 1;
        }
    }
    return 0;
}

int get_test_suite_apdu(char* u_test_file, uint8_t* data, uint8_t type, uint16_t index)
{
    if (NULL != strstr(u_test_file, _test_info.test_file))
    {
        mxml_node_t* root = _test_info.xml_root;

        mxml_node_t* test_source_node = mxmlFindElement(root, root, "test_source", "name", NULL, MXML_DESCEND);
        if (test_source_node != NULL)
        {
            mxml_node_t* node = mxmlFindElement(test_source_node, root, "apdu", NULL, NULL, MXML_DESCEND);
            if (NULL != node)
            {
                printf("mxmlGetText is %s\n", mxmlGetText(node, NULL));

                printf("mxmlElementGetAttr is %s\n", mxmlElementGetAttr(node, "apdu"));

            }
            return 1;
        }
    }
    return 0;
}

int get_test_suite_auth(char* u_test_file, uint8_t type, uint16_t index)
{
    if (NULL != strstr(u_test_file, _test_info.test_file))
    {
        mxml_node_t* root = _test_info.xml_root;

        mxml_node_t* test_source_node = mxmlFindElement(root, root, "test_source", "name", NULL, MXML_DESCEND);
        if (test_source_node != NULL)
        {
            mxml_node_t* node = mxmlFindElement(test_source_node, root, "auth", NULL, NULL, MXML_DESCEND);
            if (NULL != node)
            {
                printf("mxmlGetText is %s\n", mxmlGetText(node, NULL));

                printf("mxmlElementGetAttr is %s\n", mxmlElementGetAttr(node, "auth"));

            }
            return 1;
        }
    }
    return 0;
}
#endif