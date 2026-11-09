
#define STA_U_ALL_OVER_60UN                  0x0001
#define STA_I_ALL_OVER_5IB                   0x0002
#define STA_ACITVE_POWER_DIR_CHANGE          0x0004
#define STA_U_PHASE_SEQUENCE_REV             0x0008
#define STA_I_PHASE_SEQUENCE_REV             0x0010
#define STA_I_ALL_UNDER_5IB                  0x0020
#define STA_U_ALL_UNDER_68UN                 0x0040
#define STA_U_ALL_UNDER_60UN                 0x0080
#define STA_REACITVE_POWER_DIR_CHANGE        0x0100
#define STA_I_ALL_OVER_IMAX125               0X0200


#define  IV_PHASE_A            0
#define  IV_PHASE_B            1
#define  IV_PHASE_C            2
#define  IV_PHASE_N            3
#define  MAX_PHASE_NUM         4

enum emu_data_class_t
{
    IV_POWER_ACT,           /**< 0 有功功率 */
    IV_POWER_REACT,         /**< 1 无功功率 */
    IV_POWER_APPAR,         /**< 2 视在功率 */
    IV_VOLTAGE,             /**< 3 电压 */
    IV_CURRENT,             /**< 4 电流 */
    IV_POWFACT,             /**< 5 功率因数 */
    IV_UI_ANGEL,            /**< 6 压流角 */
    IV_U_ANGEL,             /**< 7 相角 */
    IV_FREQUENCY,           /**< 8 频率frequency */
    IV_MAX_REALCLASSNUM     
};


enum
{
    IV_ACTIVE_POWER_Positive,       /**< 正向有功 */
    IV_ACTIVE_POWER_Negative,       /**< 反向有功 */
    IV_REACTIVE_POWER_Quadrant1,    /**< 无功象限1 */
    IV_REACTIVE_POWER_Quadrant2,    /**< 无功象限2 */
    IV_REACTIVE_POWER_Quadrant3,    /**< 无功象限3 */
    IV_REACTIVE_POWER_Quadrant4,    /**< 无功象限4 */
    IV_APPARENT_POWER_Positive,     /**< 正向视在 */
    IV_APPARENT_POWER_Negative,     /**< 反向视在 */
	IV_REACTIVE_POWER_Positive,     /**< 正向无功 */
	IV_REACTIVE_POWER_Negative,     /**< 反向无功 */

	IV_ACTIVE_POWER_ABS,            /**< 有功绝对值，*/
	IV_REACTIVE_POWER_ABS,          /**< 无功绝对值，*/
	IV_APPARENT_POWER_ABS,          /**< 视在绝对值，*/
	IV_POW_CLASS_NUM
};

enum
{   
    IV_EQU = 0,/**< EQU - 等于:equal*/
    IV_NEQ = 1,/**< NEQ - 不等于:not equal*/
    IV_LSS = 2,/**< LSS - 小于:less than*/
    IV_LEQ = 3,/**< LEQ - 小于或等于:equal or less than*/
    IV_GTR = 4,/**< GTR - 大于:greater than*/
    IV_GEQ = 5,/**< GEQ - 大于或等于:equal or greater than*/
};


struct iv_spec_para_t
{
    uint16_t un;          /**< 额定电压,0.01V */
    uint16_t u_120;       /**< 120%电压，0.01V */
    uint16_t ib;          /**< 额定电流, 1毫安 */
    uint16_t u_start;     /**< 临界电压 ,0.01V */
    uint16_t u_60;        /**< 60%电压，0.01V */
    uint16_t u_68;        /**< 68%电压，0.01V */
    uint16_t i_start;     /**< 启动电流, 1毫安 */
    uint16_t ib_005;      /**< 5%额定电流，毫安 */
    uint16_t i_max;       /**< 最大电流，0.1A */
};


struct iv_control_block_t
{
    int id;
    //电量数据
    //瞬时量数据

    ///参数部分
    struct iv_spec_para_t iv_spec_para;
    uint8_t under_istart_clr;//是否清空数据

    ///瞬时量资源计数
    uint8_t iv_power_act_cnt;
    uint8_t iv_power_react_cnt;
    uint8_t iv_power_appar_cnt;
    uint8_t iv_voltage_cnt;
    uint8_t iv_current_cnt;
    uint8_t iv_powfact_cnt;
    uint8_t iv_ui_angel_cnt;
    uint8_t iv_u_angel_cnt;
    uint8_t iv_frequency_cnt;

    ///注册信息
    uint8_t *iv_power_act_cfg;
    uint8_t *iv_power_react_cfg;
    uint8_t *iv_power_appar_cfg;
    uint8_t *iv_voltage_cfg;
    uint8_t *iv_current_cfg;
    uint8_t *iv_powfact_cfg;
    uint8_t *iv_ui_angel_cfg;
    uint8_t *iv_u_angel_cfg;
    uint8_t *iv_frequency_cfg;

    ///瞬时量数据
    int32_t *iv_power_act;
    int32_t *iv_power_react;
    int32_t *iv_power_appar;
    int32_t *iv_voltage;
    int32_t *iv_current;
    int32_t *iv_powfact;
    int32_t *iv_ui_angel;
    int32_t *iv_u_angel;
    int32_t *iv_frequency;

    ///状态数据
    uint32_t state;
    ///特殊状态输出
    uint8_t over_Imax;
    uint8_t Under_75Un;
    uint8_t Under_40Un;
    uint8_t Over_120Un;

    uint8_t under_68un;
    uint8_t over_Imax;
    uint8_t under_05ib;
};

typedef int (*iv_callback_t)(struct control_block_of_app_t *app, int32_t val, uint8_t phase);


struct control_block_of_app_t
{
    int id;
    //电量数据
    //瞬时量数据
    struct iv_control_block_t *iv_control;

    //电量数据
    //瞬时量数据
    //需量数据
    //事件负荷冻结数据
    //费率数据
    //费用数据
};
   


uint8_t iv_init_action(struct control_block_of_app_t* p_app, uint8_t class, uint8_t cfg_cnt)
{
    ///资源分配
    uint8_t *p_cnt = (uint8_t *)&(p_app->iv_control->iv_power_act_cnt);
    uint8_t *p_cfg = (uint8_t *)&(p_app->iv_control->iv_power_act_cfg);
    int32_t *p_data = (int32_t *)&(p_app->iv_control->iv_power_act);

    p_cnt = p_cnt + class;
    p_data = p_data + class*4;
    *p_cnt = cfg_cnt;
    p_data = malloc(cfg_cnt*4);
    p_cfg = mallos(cfg_cnt*4);
}


uint8_t iv_add_action(struct control_block_of_app_t* p_app, uint8_t phase, uint8_t class)
{
    uint8_t reg_info = (phase << 4) + class;

    switch (class)
    {
        case IV_POWER_ACT: 
            p_app->iv_control->iv_power_act[p_app->iv_control->iv_power_act_cnt]  = reg_info;
            p_app->iv_control->iv_power_act_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_power_react[p_app->iv_control->iv_power_react_cnt] = reg_info;
            p_app->iv_control->iv_power_react_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_power_appar[p_app->iv_control->iv_power_appar_cnt] = reg_info;
            p_app->iv_control->iv_power_appar_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_voltage_val[p_app->iv_control->iv_voltage_val_cnt] = reg_info;
            p_app->iv_control->iv_voltage_val_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_current[p_app->iv_control->iv_current_cnt] = reg_info;
            p_app->iv_control->iv_current_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_powfact[p_app->iv_control->iv_powfact_cnt] = reg_info;
            p_app->iv_control->iv_powfact_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_ui_angel[p_app->iv_control->iv_ui_angel_cnt] = reg_info;
            p_app->iv_control->iv_ui_angel_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_u_angel[p_app->iv_control->iv_u_angel_cnt] = reg_info;
            p_app->iv_control->iv_u_angel_cnt++;
            break;
        case IV_POWER_REACT: 
            p_app->iv_control->iv_frequency[p_app->iv_control->iv_frequency_cnt] = reg_info;
            p_app->iv_control->iv_frequency_cnt++;
        default:
             break;
    }
}


static int32_t _iv_get_phase_val(struct control_block_of_app_t* p_app, uint8_t phase, uint8_t class)
{
    uint8_t i = 0;
    uint8_t phase_x = 0;
    uint8_t class_x = 0;
    uint16_t val = 0;

    for(i =0 ; i < p_block->reg_cfg_cnt; i++)
    {
        if(p_block->reg_cfg[i] == (phase << 4) + class)
        {
            return p_block->emu_val[i];
        }
    }
    return 0;
}

static int32_t _iv_set_phase_val(struct control_block_of_app_t* p_app, uint8_t phase, uint8_t class，int32_t val)
{
    uint8_t i = 0;
    uint8_t phase_x = 0;
    uint8_t class_x = 0;
    uint16_t val = 0;

    for(i =0 ; i < p_block->reg_cfg_cnt; i++)
    {
        if(p_block->reg_cfg[i] == (phase << 4) + class)
        {
            p_block->emu_val[i] = val;
        }
    }
    return 0;
}


//////////////////////////////////////////////////////
static void iv_voltage_start_handle(struct control_block_of_app_t* p_app,, int32_t un, )
{
    if(un < p_block->iv_spec_para.u_start)
    {
        if (p_block->under_istart_clr != 0xAA)
        {            
            which_instantaneous->real[FREORVOL]  = 0;
            _iv_set_phase_val(p_block, 0);
            which_instantaneous->real[POW_ACT]   = 0;
            _iv_set_phase_val(p_block, 0);
            which_instantaneous->real[POW_APPAR] = 0;
            _iv_set_phase_val(p_block, 0);
            which_instantaneous->real[POWFACT]   = REAL_PF_DEFAULT;
            _iv_set_phase_val(p_block, REAL_PF_DEFAULT);
        }        
    }
}

static void __iv_data_handle(int32_t un, uint8_t type, int32_t val, uint8 *flg)
{
    *flg = 0;
    if( ((IV_LSS == type) && (hfp_abs(val) <  un)) ||
        ((IV_LEQ == type) && (hfp_abs(val) <= un)) ||
        ((IV_GTR == type) && (hfp_abs(val) >  un)) ||
        ((IV_GEQ == type) && (hfp_abs(val) >= un)))
    {
        *flg = 0x55;
    }
}


uint8_t iv_voltage_update(struct control_block_of_app_t* p_app,  int32_t val, uint8_t phase)
{
    if(which_instantaneous->real[POW_ACT] < 0)//反向
    {
        which_instantaneous->real[CURRENT] = ~which_instantaneous->real[CURRENT];
    }

    iv_voltage_start_handle(p_block, val);
    __iv_data_handle(75*p_block->iv_spec_para.un)/100, IV_LSS, val, &p_block->Under_75Un[phase]);
    __iv_data_handle(40*p_block->iv_spec_para.un)/100, IV_LSS, val, &p_block->Under_40Un[phase]);

    //u_60
    __iv_data_handle(p_block->iv_spec_para.u_60, IV_LSS, val, &p_block->under_60un[phase]);
    __iv_data_handle(p_block->iv_spec_para.u_68, IV_LSS, val, &p_block->under_68un[phase]);
    __iv_data_handle(p_block->iv_spec_para.u_120, IV_GTR, val, &p_block->Over_120Un[phase]);
}


static void iv_current_start_handle(struct control_block_of_app_t* p_app, int32_t un, )
{
    abs_current = hfp_abs(val);
    if(abs_current < p_block->iv_spec_para.i_start)
    {
        which_instantaneous->real[CURRENT] = 0;
    }

		abs_current = hfp_abs(which_instantaneous->real[CURRENT]);
		if(abs_current < specification_parameter.i_start)
		{  
            if (under_istart_clr != 0xAA)
            {
    			which_instantaneous->real[CURRENT] 	 = 0;
    			which_instantaneous->real[POW_ACT]   = 0;
                which_instantaneous->real[POW_APPAR] = 0;
                which_instantaneous->real[POWFACT]   = REAL_PF_DEFAULT;
    			no_current |= not_mask;
            }
		}
		else
		{
			no_current &= mask;
		}
		if(no_current & 0x01)
		{
			which_instantaneous->real[POW_ACT]   = 0;
            which_instantaneous->real[POW_APPAR] = 0;
            which_instantaneous->real[POWFACT]   = REAL_PF_DEFAULT;
		}
        
}

uint8_t iv_current_update(struct control_block_of_app_t* p_app,  int32_t val, uint8_t phase)
{
    //启动电流处理
    iv_current_start_handle();

    ///1.25 i_max
    __iv_data_handle(1250*p_block->iv_spec_para.i_max, IV_GTR, val, &p_block->over_Imax[phase]);

    ///ib_005
    __iv_data_handle(p_block->iv_spec_para.ib_005, IV_GTR, val, &p_block->under_05ib[phase]);

    if((abs(val) < p_block->iv_spec_para.ib_005))
    {
        if(which_instantaneous->real[FREORVOL]>0)
        {
            s=(float)which_instantaneous->real[POW_APPAR]*10000/which_instantaneous->real[FREORVOL];
            which_instantaneous->real[CURRENT]=(int32)s;
        }
    }

    /*
        if(under_05ib==0)
        {
            which_instantaneous->state |=STA_I_ALL_OVER_5IB;
        }
        else if(under_05ib ==(MEASURE_PHASE_BIT_MAP & mask_type))
        {
            which_instantaneous->state |=STA_I_ALL_UNDER_5IB;
        }
    */
}

uint8_t iv_powact_update(struct control_block_of_app_t* p_app,  int32_t val, uint8_t phase)
{
    if((which_instantaneous->real[POW_ACT] < 0)&&(last_power_dir > 0))
    {
        which_instantaneous->state |= STA_ACITVE_POWER_DIR_CHANGE;
        last_power_dir=0xff;
    }
    if((which_instantaneous->real[POW_ACT]>0)&&(last_power_dir<0))
    {
        which_instantaneous->state |= STA_ACITVE_POWER_DIR_CHANGE;
        last_power_dir=0x01;
    }
}


uint8_t _iv_data_update(struct control_block_of_app_t* p_app,  uint8_t cnt, int32_t *data, uint8_t *cfg, iv_callback_t iv_cb)
{
    uint8_t i = 0;

    for(i =0 ; i < cnt; i++)
    {
        val   = data[i];
        phase = (cfg[i] & 0x0f)>>4;
        if(iv_cb)
            iv_cb(p_block, val, phase);
    }   
}


uint8_t iv_data_update(struct control_block_of_app_t* p_app, uint8_t phase)
{
    struct iv_control_block_t *p_iv_control;

    if(NULL == p_app)
        return 0;

    p_iv_control = p_app->iv_control;

    ///IV_POWER_ACT 有功功率
    _iv_data_update(p_app, p_iv_control->iv_power_act_cnt, p_iv_control->iv_power_act, p_iv_control->iv_power_act_cfg, iv_powact_update);

    ///IV_POWER_REACT 无功功率
    ///IV_POWER_APPAR 视在功率

    ///IV_VOLTAGE 电压处理 ok
    _iv_data_update(p_app, p_iv_control->iv_voltage_cnt, p_iv_control->iv_voltage, p_iv_control->iv_voltage_cfg, iv_voltage_update);

    ///IV_CURRENT 电流处理 ok
    _iv_data_update(p_app, p_iv_control->iv_current_cnt, p_iv_control->iv_current, p_iv_control->iv_current_cfg, iv_current_update);

    ///IV_POWFACT 功率因数
    ///IV_UI_ANGEL 压流角
    ///IV_U_ANGEL 相角
    ///IV_FREQUENCY 频率
}




int baremetal_instance_entry(int argc, char** argv)
{
    struct instance_t *instance;
    //初始化硬件
    //初始化固件
    //初始化软件
        //每个application的control block如何初始化?且如何绑定到instance里?
        //每个application如何访问自己的control block?


        //三相瞬时量
        //1. app1的字段初始化
        ///app1.id = 1;//初始化app_id
        ///app1.std_in = &fifo1;//指定标准输入
        //初始化control block 添加action  裁剪时 去掉对应的action  则响应的action的资源自动排除
        ///control_block_of_app_1 
        struct control_block_of_app_t control_block_of_app_1;
        //增加某相某类  分配相应的资源 同时更新resource manager
        iv_init_action(&control_block_of_app_1, IV_POWER_ACT, 3);
        iv_add_action(&control_block_of_app_1, IV_PHASE_A, IV_POWER_ACT);
        iv_add_action(&control_block_of_app_1, IV_PHASE_B, IV_POWER_ACT);
        iv_add_action(&control_block_of_app_1, IV_PHASE_C, IV_POWER_ACT);

        iv_init_action((&control_block_of_app_1, IV_POWER_REACT, 3);
        iv_add_action(&control_block_of_app_1, IV_PHASE_A, IV_POWER_REACT);
        iv_add_action(&control_block_of_app_1, IV_PHASE_B, IV_POWER_REACT);
        iv_add_action(&control_block_of_app_1, IV_PHASE_C, IV_POWER_REACT);

        iv_init_action((&control_block_of_app_1, IV_POWER_APPAR, 3);
        iv_add_action(&control_block_of_app_1, IV_PHASE_A, IV_POWER_APPAR);
        iv_add_action(&control_block_of_app_1, IV_PHASE_B, IV_POWER_APPAR);
        iv_add_action(&control_block_of_app_1, IV_PHASE_C, IV_POWER_APPAR);

        iv_init_action((&control_block_of_app_1, IV_VOLTAGE, 3);
        iv_add_action(&control_block_of_app_1, IV_PHASE_A, IV_VOLTAGE);
        iv_add_action(&control_block_of_app_1, IV_PHASE_B, IV_VOLTAGE);
        iv_add_action(&control_block_of_app_1, IV_PHASE_C, IV_VOLTAGE);

        iv_init_action((&control_block_of_app_1, IV_CURRENT, 3);
        iv_add_action(&control_block_of_app_1, IV_PHASE_A, IV_CURRENT);
        iv_add_action(&control_block_of_app_1, IV_PHASE_B, IV_CURRENT);
        iv_add_action(&control_block_of_app_1, IV_PHASE_C, IV_CURRENT);

        tx_pipeline_create(&app1,app1_entry,&control_block_of_app_1);//初始化完成app1 绑定三元组
        add_app(instance,&app1);


         //单相瞬时量
        app2.id = 1;//初始化app_id
        app2.std_in = &fifo1;//指定标准输入
        //初始化control block 添加action  裁剪时 去掉对应的action  则响应的action的资源自动排除
        //增加某相某类  分配相应的资源 同时更新resource manager

        iv_init_action((&control_block_of_app_1, IV_POWER_ACT, 3);
        iv_add_action(&control_block_of_app_2, IV_PHASE_A, IV_POWER_ACT);

        iv_init_action((&control_block_of_app_1, IV_POWER_REACT, 3);
        iv_add_action(&control_block_of_app_2, IV_PHASE_A, IV_POWER_REACT);

        iv_init_action((&control_block_of_app_1, IV_POWER_APPAR, 3);
        iv_add_action(&control_block_of_app_2, IV_PHASE_A, IV_POWER_APPAR);

        iv_init_action((&control_block_of_app_1, IV_VOLTAGE, 3);
        iv_add_action(&control_block_of_app_2, IV_PHASE_A, IV_VOLTAGE);

        iv_init_action((&control_block_of_app_1, IV_CURRENT, 3);
        iv_add_action(&control_block_of_app_2, IV_PHASE_A, IV_CURRENT);


        tx_pipeline_create(&app2,app2_entry,&control_block_of_app_2);//初始化完成app1 绑定三元组
        add_app(instance,&app2);

    //链接和绑定硬件 固件 和 软件
    //运行主循环
}

