#include "stdafx.h"
#include<Windows.h>

//窗口过程回调函数
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
//相当于标准C语言中的 int main()
int _stdcall WinMain(HINSTANCE hIstance, //当前实例句柄  相当于nidavellir里面的instance_t
    HINSTANCE hPrevInstance, //前个实例句柄，win16遗留下来的废物，不用但必须要写
    LPSTR lpszCmdLine,
    int nCmdShow)
{
    //声明窗口句柄
    HWND hwnd;
    //声明消息结构
    MSG msg;
    //声明窗口类
    WNDCLASS wc;

    //1.设计一个窗口类
    wc.style = 0; //窗口样式，0表示默认样式
    
    //这里是注册消息处理函数  类似nidavellir里面的pipeline的入口函数
    wc.lpfnWndProc = (WNDPROC)WndProc;//窗口过程，告诉WndProc窗口过程函数，你要处理我的消息

    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hIstance;//当前实例句柄，就是wc是哪个实例的窗口
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);//图标
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);//鼠标样式
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);//背景颜色，用COLOR_WINDOW这种颜色来刷背景
    wc.lpszMenuName = NULL; //菜单
    wc.lpszClassName = TEXT("MyWndClass"); //设置窗口类的名称

    //2.注册窗口类
    RegisterClass(&wc);

    //以上的内容相当于nidavellir里面的recipe

    //3.创建窗口   这个相当于nidavellir里面的struct instance_t *create_nidavellir(struct recipe_t *recipe)
    hwnd = CreateWindow(
        TEXT("MyWndClass"),//窗口标题
        TEXT("Hello SDK Application"),
        WS_OVERLAPPEDWINDOW,//窗口样式
        CW_USEDEFAULT,//窗口显示x坐标位置，CW_USEDEFAULT是使用默认的位置
        CW_USEDEFAULT,//窗口显示y坐标位置，CW_USEDEFAULT是使用默认的位置
        CW_USEDEFAULT,//窗口的宽度，CW_USEDEFAULT是使用默认的位置
        CW_USEDEFAULT,//窗口的高度，CW_USEDEFAULT是使用默认的位置
        NULL,//该窗口的父窗口，NULL是没有父窗口，也就是该窗口就是顶级窗口
        NULL,//窗口菜单句柄
        hIstance,
        NULL
    );

    //4.显示和更新窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    //5.消息循环  这个相当于nidavellir里面的nidavellir_run  如下代码片段
    /*  
        while(fifoBuf_getUsed(pipeline->in_queue) >= sizeof(struct message_t ))//waitmesg
        {
            filter = pipeline->filters;
            n = pipeline->filter_n;
            do
            {
                ret = filter->on_work(instance, pipeline, &mesg);
            }
        }
    */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);//翻译消息
        DispatchMessage(&msg);//转发到窗口过程进行处理
    }

    return msg.lParam;

}

LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    PAINTSTRUCT ps;//声明刷子
    HDC hdc; //dc句柄
    RECT rect;
    //对各种消息进行处理
    switch (message)
    {
    case WM_SIZE:
        //重画Paint
        break;
    case  WM_LBUTTONDOWN://鼠标左击事件
        MessageBox(hwnd,TEXT("Mouse Click!"),TEXT("Tip"),MB_OK);
        break;
    case WM_PAINT: //绘制消息
        hdc = BeginPaint(hwnd,&ps);
        GetClientRect(hwnd,&rect);//获取hwnd窗口的客户区矩形大小
        Ellipse(hdc,0,0,200,100);//绘制一个椭圆
        DrawText(hdc,TEXT("HELLO WORLD"),-1,&rect,DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        EndPaint(hwnd,&ps);
        break;

    case WM_COMMAND:

        break;

    case WM_DESTROY://销毁窗口（关闭窗口）时执行
        PostQuitMessage(0);
        break;
    }

    //我们不处理的消息就转给window处理
    return DefWindowProc(hwnd, message, wParam, lParam);
}