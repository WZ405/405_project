
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx299_sensor_ctl.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/11/07
  Description   :
  History       :
  1.Date        : 2013/11/07
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

//#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
//#include "drv_i2c.h"
#include "hi_i2c.h"

const unsigned char imx299_i2c_addr     =    0x34;       /* I2C Address of IMX299 */
const unsigned int  imx299_addr_byte    =    2;
const unsigned int  imx299_data_byte    =    1;
static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};


extern ISP_SNS_STATE_S    g_astImx299[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U  g_aunImx299BusInfo[];

int imx299_i2c_init(VI_PIPE ViPipe)
{
    int ret;
    char acDevFile[16];
    HI_U8 u8DevNum;

    if (g_fd[ViPipe] >= 0)
    {
        return 0;
    }

    u8DevNum = g_aunImx299BusInfo[ViPipe].s8I2cDev;

    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

    g_fd[ViPipe] = open(acDevFile, O_RDWR);

    if (g_fd[ViPipe] < 0)
    {
        printf("Open /dev/hi_i2c_drv-%u error!\n", u8DevNum);
        return -1;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (imx299_i2c_addr >> 1));
    if (ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }

    return 0;
}

int imx299_i2c_exit(VI_PIPE ViPipe)
{
    if (g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return 0;
    }
    return -1;
}

int imx299_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:

    return 0;
}

int imx299_write_register(VI_PIPE ViPipe, int addr, int data)
{
    int ret;
    int idx = 0;
    char buf[8];

    if (0 > g_fd[ViPipe])
    {
        return 0;
    }

    if (imx299_addr_byte == 2)
    {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    }
    else
    {
        buf[idx] = addr & 0xff;
        idx++;
    }

    if (imx299_data_byte == 2)
    {
        buf[idx] = (data >> 8) & 0xff;
        idx++;
        buf[idx] = data & 0xff;
        idx++;
    }
    else
    {
        buf[idx] = data & 0xff;
        idx++;
    }

    ret = write(g_fd[ViPipe], buf, (imx299_addr_byte + imx299_data_byte));
    if (ret < 0)
    {
        printf("I2C_WRITE DATA error!\n");
        return -1;
    }

    return 0;
}

static void delay_ms(int ms)
{
    usleep(ms * 1000);
}

void imx299_standby(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

void imx299_restart(VI_PIPE ViPipe)
{
    // TODO:
    return;
}


void imx299_wdr_init(VI_PIPE ViPipe);
void imx299_linear_8Mp30_init(VI_PIPE ViPipe);

void imx299_init(VI_PIPE ViPipe)
{
    WDR_MODE_E       enWDRMode;
    //HI_U8            u8ImgMode;
    HI_BOOL          bInit;

    bInit       = g_astImx299[ViPipe].bInit;
    enWDRMode   = g_astImx299[ViPipe].enWDRMode;
    //u8ImgMode   = g_astImx299[ViPipe].u8ImgMode;

    if (imx299_i2c_init(ViPipe))
    {
        return;
    }

    /* When imx299 first init, config all registers */
    if (HI_FALSE == bInit)
    {
        if (WDR_MODE_2To1_LINE == enWDRMode)
        {
            imx299_wdr_init(ViPipe);
        }
        else
        {
            imx299_linear_8Mp30_init(ViPipe);
        }
    }
    /* When imx299 switch mode(linear<->WDR or resolution), config different registers(if possible) */
    else
    {
        if (WDR_MODE_2To1_LINE == enWDRMode)
        {
            imx299_wdr_init(ViPipe);
        }
        else
        {
            imx299_linear_8Mp30_init(ViPipe);
        }
    }

    g_astImx299[ViPipe].bInit = HI_TRUE;

    return ;
}

void imx299_exit(VI_PIPE ViPipe)
{
    imx299_i2c_exit(ViPipe);

    return;
}

/* 4096*2160 @30fps */
void imx299_linear_8Mp30_init(VI_PIPE ViPipe)
{
    imx299_write_register (ViPipe, 0x3033, 0x30);
    imx299_write_register (ViPipe, 0x303C, 0x02);
    imx299_write_register (ViPipe, 0x311F, 0x00);
    imx299_write_register (ViPipe, 0x3122, 0x02);
    imx299_write_register (ViPipe, 0x3123, 0x00);
    imx299_write_register (ViPipe, 0x3124, 0x00);
    imx299_write_register (ViPipe, 0x3125, 0x01);
    imx299_write_register (ViPipe, 0x3127, 0x02);
    imx299_write_register (ViPipe, 0x3129, 0x90);
    imx299_write_register (ViPipe, 0x312A, 0x02);
    imx299_write_register (ViPipe, 0x312D, 0x02);
    imx299_write_register (ViPipe, 0x31E8, 0xF0);
    imx299_write_register (ViPipe, 0x31E9, 0x00);
    imx299_write_register (ViPipe, 0x3134, 0xA7);
    imx299_write_register (ViPipe, 0x3135, 0x00);
    imx299_write_register (ViPipe, 0x3136, 0x9F);
    imx299_write_register (ViPipe, 0x3137, 0x00);
    imx299_write_register (ViPipe, 0x3138, 0x6F);
    imx299_write_register (ViPipe, 0x3139, 0x00);
    imx299_write_register (ViPipe, 0x313A, 0x5F);
    imx299_write_register (ViPipe, 0x313B, 0x00);
    imx299_write_register (ViPipe, 0x313C, 0x5F);
    imx299_write_register (ViPipe, 0x313D, 0x00);
    imx299_write_register (ViPipe, 0x313E, 0x7F);
    imx299_write_register (ViPipe, 0x313F, 0x01);
    imx299_write_register (ViPipe, 0x3140, 0x6F);
    imx299_write_register (ViPipe, 0x3141, 0x00);
    imx299_write_register (ViPipe, 0x3142, 0x4F);
    imx299_write_register (ViPipe, 0x3143, 0x00);
    imx299_write_register (ViPipe, 0x3000, 0x12);
    imx299_write_register (ViPipe, 0x310B, 0x00);
    imx299_write_register (ViPipe, 0x3047, 0x01);
    imx299_write_register (ViPipe, 0x304E, 0x0B);
    imx299_write_register (ViPipe, 0x304F, 0x24);
    imx299_write_register (ViPipe, 0x3062, 0x25);
    imx299_write_register (ViPipe, 0x3064, 0x78);
    imx299_write_register (ViPipe, 0x3065, 0x33);
    imx299_write_register (ViPipe, 0x3067, 0x71);
    imx299_write_register (ViPipe, 0x3088, 0x75);
    imx299_write_register (ViPipe, 0x308A, 0x09);
    imx299_write_register (ViPipe, 0x308B, 0x01);
    imx299_write_register (ViPipe, 0x308C, 0x61);
    imx299_write_register (ViPipe, 0x3146, 0x00);
    imx299_write_register (ViPipe, 0x3234, 0x32);
    imx299_write_register (ViPipe, 0x3235, 0x00);
    imx299_write_register (ViPipe, 0x3248, 0xBC);
    imx299_write_register (ViPipe, 0x3249, 0x00);
    imx299_write_register (ViPipe, 0x3250, 0xBC);
    imx299_write_register (ViPipe, 0x3251, 0x00);
    imx299_write_register (ViPipe, 0x3258, 0xBC);
    imx299_write_register (ViPipe, 0x3259, 0x00);
    imx299_write_register (ViPipe, 0x3260, 0xBC);
    imx299_write_register (ViPipe, 0x3261, 0x00);
    imx299_write_register (ViPipe, 0x3274, 0x13);
    imx299_write_register (ViPipe, 0x3275, 0x00);
    imx299_write_register (ViPipe, 0x3276, 0x1F);
    imx299_write_register (ViPipe, 0x3277, 0x00);
    imx299_write_register (ViPipe, 0x3278, 0x30);
    imx299_write_register (ViPipe, 0x3279, 0x00);
    imx299_write_register (ViPipe, 0x327C, 0x13);
    imx299_write_register (ViPipe, 0x327D, 0x00);
    imx299_write_register (ViPipe, 0x327E, 0x1F);
    imx299_write_register (ViPipe, 0x327F, 0x00);
    imx299_write_register (ViPipe, 0x3280, 0x30);
    imx299_write_register (ViPipe, 0x3281, 0x00);
    imx299_write_register (ViPipe, 0x3284, 0x13);
    imx299_write_register (ViPipe, 0x3285, 0x00);
    imx299_write_register (ViPipe, 0x3286, 0x1F);
    imx299_write_register (ViPipe, 0x3287, 0x00);
    imx299_write_register (ViPipe, 0x3288, 0x30);
    imx299_write_register (ViPipe, 0x3289, 0x00);
    imx299_write_register (ViPipe, 0x328C, 0x13);
    imx299_write_register (ViPipe, 0x328D, 0x00);
    imx299_write_register (ViPipe, 0x328E, 0x1F);
    imx299_write_register (ViPipe, 0x328F, 0x00);
    imx299_write_register (ViPipe, 0x3290, 0x30);
    imx299_write_register (ViPipe, 0x3291, 0x00);
    imx299_write_register (ViPipe, 0x32AE, 0x00);
    imx299_write_register (ViPipe, 0x32AF, 0x00);
    imx299_write_register (ViPipe, 0x32CA, 0x5A);
    imx299_write_register (ViPipe, 0x32CB, 0x00);
    imx299_write_register (ViPipe, 0x332F, 0x00);
    imx299_write_register (ViPipe, 0x334C, 0x01);
    imx299_write_register (ViPipe, 0x335A, 0x79);
    imx299_write_register (ViPipe, 0x335B, 0x00);
    imx299_write_register (ViPipe, 0x335E, 0x56);
    imx299_write_register (ViPipe, 0x335F, 0x00);
    imx299_write_register (ViPipe, 0x3360, 0x6A);
    imx299_write_register (ViPipe, 0x3361, 0x00);
    imx299_write_register (ViPipe, 0x336A, 0x56);
    imx299_write_register (ViPipe, 0x336B, 0x00);
    imx299_write_register (ViPipe, 0x33D6, 0x79);
    imx299_write_register (ViPipe, 0x33D7, 0x00);
    imx299_write_register (ViPipe, 0x340C, 0x6E);
    imx299_write_register (ViPipe, 0x340D, 0x00);
    imx299_write_register (ViPipe, 0x3448, 0x7E);
    imx299_write_register (ViPipe, 0x3449, 0x00);
    imx299_write_register (ViPipe, 0x348E, 0x6F);
    imx299_write_register (ViPipe, 0x348F, 0x00);
    imx299_write_register (ViPipe, 0x3492, 0x11);
    imx299_write_register (ViPipe, 0x34C4, 0x5A);
    imx299_write_register (ViPipe, 0x34C5, 0x00);
    imx299_write_register (ViPipe, 0x3506, 0x56);
    imx299_write_register (ViPipe, 0x3507, 0x00);
    imx299_write_register (ViPipe, 0x350C, 0x56);
    imx299_write_register (ViPipe, 0x350D, 0x00);
    imx299_write_register (ViPipe, 0x350E, 0x58);
    imx299_write_register (ViPipe, 0x350F, 0x00);
    imx299_write_register (ViPipe, 0x3549, 0x04);
    imx299_write_register (ViPipe, 0x355D, 0x03);
    imx299_write_register (ViPipe, 0x355E, 0x03);
    imx299_write_register (ViPipe, 0x3574, 0x56);
    imx299_write_register (ViPipe, 0x3575, 0x00);
    imx299_write_register (ViPipe, 0x3587, 0x01);
    imx299_write_register (ViPipe, 0x35D0, 0x5E);
    imx299_write_register (ViPipe, 0x35D1, 0x00);
    imx299_write_register (ViPipe, 0x35D4, 0x63);
    imx299_write_register (ViPipe, 0x35D5, 0x00);
    imx299_write_register (ViPipe, 0x366A, 0x1A);
    imx299_write_register (ViPipe, 0x366B, 0x16);
    imx299_write_register (ViPipe, 0x366C, 0x10);
    imx299_write_register (ViPipe, 0x366D, 0x09);
    imx299_write_register (ViPipe, 0x366E, 0x00);
    imx299_write_register (ViPipe, 0x366F, 0x00);
    imx299_write_register (ViPipe, 0x3670, 0x00);
    imx299_write_register (ViPipe, 0x3671, 0x00);
    imx299_write_register (ViPipe, 0x3676, 0x83);
    imx299_write_register (ViPipe, 0x3677, 0x03);
    imx299_write_register (ViPipe, 0x3678, 0x00);
    imx299_write_register (ViPipe, 0x3679, 0x04);
    imx299_write_register (ViPipe, 0x367A, 0x2C);
    imx299_write_register (ViPipe, 0x367B, 0x05);
    imx299_write_register (ViPipe, 0x367C, 0x00);
    imx299_write_register (ViPipe, 0x367D, 0x06);
    imx299_write_register (ViPipe, 0x367E, 0x00);
    imx299_write_register (ViPipe, 0x367F, 0x07);
    imx299_write_register (ViPipe, 0x3680, 0x4B);
    imx299_write_register (ViPipe, 0x3681, 0x07);
    imx299_write_register (ViPipe, 0x3690, 0x27);
    imx299_write_register (ViPipe, 0x3691, 0x00);
    imx299_write_register (ViPipe, 0x3692, 0x65);
    imx299_write_register (ViPipe, 0x3693, 0x00);
    imx299_write_register (ViPipe, 0x3694, 0x4F);
    imx299_write_register (ViPipe, 0x3695, 0x00);
    imx299_write_register (ViPipe, 0x3696, 0xA1);
    imx299_write_register (ViPipe, 0x3697, 0x00);
    imx299_write_register (ViPipe, 0x382B, 0x68);
    imx299_write_register (ViPipe, 0x3C00, 0x01);
    imx299_write_register (ViPipe, 0x3C01, 0x01);
    imx299_write_register (ViPipe, 0x3686, 0x00);
    imx299_write_register (ViPipe, 0x3687, 0x00);
    imx299_write_register (ViPipe, 0x36BE, 0x01);
    imx299_write_register (ViPipe, 0x36BF, 0x00);
    imx299_write_register (ViPipe, 0x36C0, 0x01);
    imx299_write_register (ViPipe, 0x36C1, 0x00);
    imx299_write_register (ViPipe, 0x36C2, 0x01);
    imx299_write_register (ViPipe, 0x36C3, 0x00);
    imx299_write_register (ViPipe, 0x3004, 0x1a);
    imx299_write_register (ViPipe, 0x3005, 0x06);
    imx299_write_register (ViPipe, 0x3006, 0x00);
    imx299_write_register (ViPipe, 0x3007, 0xA0);
    imx299_write_register (ViPipe, 0x300E, 0x00);
    imx299_write_register (ViPipe, 0x300F, 0x00);
    imx299_write_register (ViPipe, 0x3018, 0x00);
    imx299_write_register (ViPipe, 0x3019, 0x00);
    imx299_write_register (ViPipe, 0x302C, 0x05);
    imx299_write_register (ViPipe, 0x302D, 0x00);
    imx299_write_register (ViPipe, 0x3030, 0x77);
    imx299_write_register (ViPipe, 0x3034, 0x00);
    imx299_write_register (ViPipe, 0x3035, 0x01);
    imx299_write_register (ViPipe, 0x3036, 0x30);
    imx299_write_register (ViPipe, 0x3037, 0x00);
    imx299_write_register (ViPipe, 0x3038, 0x60);
    imx299_write_register (ViPipe, 0x3039, 0x10);
    imx299_write_register (ViPipe, 0x3068, 0x1a);
    imx299_write_register (ViPipe, 0x3069, 0x00);
    imx299_write_register (ViPipe, 0x3080, 0x00);
    imx299_write_register (ViPipe, 0x3081, 0x01);
    imx299_write_register (ViPipe, 0x3084, 0x55);
    imx299_write_register (ViPipe, 0x3085, 0x05);
    imx299_write_register (ViPipe, 0x3086, 0x55);
    imx299_write_register (ViPipe, 0x3087, 0x05);
    imx299_write_register (ViPipe, 0x30A8, 0x02);
    imx299_write_register (ViPipe, 0x30a9, 0xe0);
    imx299_write_register (ViPipe, 0x30aa, 0x06);
    imx299_write_register (ViPipe, 0x30ab, 0x00);
    imx299_write_register (ViPipe, 0x30ac, 0x55);
    imx299_write_register (ViPipe, 0x30ad, 0x05);
    imx299_write_register (ViPipe, 0x30E2, 0x00);
    imx299_write_register (ViPipe, 0x312F, 0x08);
    imx299_write_register (ViPipe, 0x3130, 0x88);
    imx299_write_register (ViPipe, 0x3131, 0x08);
    imx299_write_register (ViPipe, 0x3132, 0x80);
    imx299_write_register (ViPipe, 0x3133, 0x08);
    imx299_write_register (ViPipe, 0x332c, 0x89);
    imx299_write_register (ViPipe, 0x332d, 0x02);
    imx299_write_register (ViPipe, 0x334a, 0x89);
    imx299_write_register (ViPipe, 0x334b, 0x02);
    imx299_write_register (ViPipe, 0x357F, 0x0C);
    imx299_write_register (ViPipe, 0x3580, 0x0A);
    imx299_write_register (ViPipe, 0x3581, 0x08);
    imx299_write_register (ViPipe, 0x3583, 0x72);
    imx299_write_register (ViPipe, 0x35b6, 0x89);
    imx299_write_register (ViPipe, 0x35b7, 0x02);
    imx299_write_register (ViPipe, 0x35b8, 0x84);
    imx299_write_register (ViPipe, 0x35b9, 0x02);
    imx299_write_register (ViPipe, 0x36bc, 0x89);
    imx299_write_register (ViPipe, 0x36bd, 0x02);
    imx299_write_register (ViPipe, 0x3846, 0x00);
    imx299_write_register (ViPipe, 0x3847, 0x00);
    imx299_write_register (ViPipe, 0x384A, 0x00);
    imx299_write_register (ViPipe, 0x384B, 0x00);
    delay_ms(10);
    imx299_write_register (ViPipe, 0x3000, 0x02);
    imx299_write_register (ViPipe, 0x35E5, 0x92);
    imx299_write_register (ViPipe, 0x35E5, 0x9A);
    imx299_write_register (ViPipe, 0x3000, 0x00);
    delay_ms(7);
    imx299_write_register (ViPipe, 0x3033, 0x20);
    imx299_write_register (ViPipe, 0x3017, 0xA8);

    printf("===sony imx299 sensor 4096x2160@30fps linear mode(MIPI port) init success!=====\n");

    return;
}

void imx299_wdr_init(VI_PIPE ViPipe)
{
    imx299_write_register(ViPipe, 0x3033, 0x30);
    imx299_write_register(ViPipe, 0x303C, 0x02);
    imx299_write_register(ViPipe, 0x311F, 0x00);
    imx299_write_register(ViPipe, 0x3122, 0x02);
    imx299_write_register(ViPipe, 0x3123, 0x00);
    imx299_write_register(ViPipe, 0x3124, 0x00);
    imx299_write_register(ViPipe, 0x3125, 0x01);
    imx299_write_register(ViPipe, 0x3127, 0x02);
    imx299_write_register(ViPipe, 0x3129, 0x90);
    imx299_write_register(ViPipe, 0x312A, 0x02);
    imx299_write_register(ViPipe, 0x312D, 0x02);
    imx299_write_register(ViPipe, 0x31E8, 0xF0);
    imx299_write_register(ViPipe, 0x31E9, 0x00);
    imx299_write_register(ViPipe, 0x3134, 0xA7);
    imx299_write_register(ViPipe, 0x3135, 0x00);
    imx299_write_register(ViPipe, 0x3136, 0x9F);
    imx299_write_register(ViPipe, 0x3137, 0x00);
    imx299_write_register(ViPipe, 0x3138, 0x6F);
    imx299_write_register(ViPipe, 0x3139, 0x00);
    imx299_write_register(ViPipe, 0x313A, 0x5F);
    imx299_write_register(ViPipe, 0x313B, 0x00);
    imx299_write_register(ViPipe, 0x313C, 0x5F);
    imx299_write_register(ViPipe, 0x313D, 0x00);
    imx299_write_register(ViPipe, 0x313E, 0x7F);
    imx299_write_register(ViPipe, 0x313F, 0x01);
    imx299_write_register(ViPipe, 0x3140, 0x6F);
    imx299_write_register(ViPipe, 0x3141, 0x00);
    imx299_write_register(ViPipe, 0x3142, 0x4F);
    imx299_write_register(ViPipe, 0x3143, 0x00);
    imx299_write_register(ViPipe, 0x3000, 0x12);
    imx299_write_register(ViPipe, 0x310B, 0x00);
    imx299_write_register(ViPipe, 0x3047, 0x01);
    imx299_write_register(ViPipe, 0x304E, 0x0B);
    imx299_write_register(ViPipe, 0x304F, 0x24);
    imx299_write_register(ViPipe, 0x3062, 0x25);
    imx299_write_register(ViPipe, 0x3064, 0x78);
    imx299_write_register(ViPipe, 0x3065, 0x33);
    imx299_write_register(ViPipe, 0x3067, 0x71);
    imx299_write_register(ViPipe, 0x3088, 0x75);
    imx299_write_register(ViPipe, 0x308A, 0x09);
    imx299_write_register(ViPipe, 0x308B, 0x01);
    imx299_write_register(ViPipe, 0x308C, 0x61);
    imx299_write_register(ViPipe, 0x3146, 0x00);
    imx299_write_register(ViPipe, 0x3234, 0x32);
    imx299_write_register(ViPipe, 0x3235, 0x00);
    imx299_write_register(ViPipe, 0x3248, 0xBC);
    imx299_write_register(ViPipe, 0x3249, 0x00);
    imx299_write_register(ViPipe, 0x3250, 0xBC);
    imx299_write_register(ViPipe, 0x3251, 0x00);
    imx299_write_register(ViPipe, 0x3258, 0xBC);
    imx299_write_register(ViPipe, 0x3259, 0x00);
    imx299_write_register(ViPipe, 0x3260, 0xBC);
    imx299_write_register(ViPipe, 0x3261, 0x00);
    imx299_write_register(ViPipe, 0x3274, 0x13);
    imx299_write_register(ViPipe, 0x3275, 0x00);
    imx299_write_register(ViPipe, 0x3276, 0x1F);
    imx299_write_register(ViPipe, 0x3277, 0x00);
    imx299_write_register(ViPipe, 0x3278, 0x30);
    imx299_write_register(ViPipe, 0x3279, 0x00);
    imx299_write_register(ViPipe, 0x327C, 0x13);
    imx299_write_register(ViPipe, 0x327D, 0x00);
    imx299_write_register(ViPipe, 0x327E, 0x1F);
    imx299_write_register(ViPipe, 0x327F, 0x00);
    imx299_write_register(ViPipe, 0x3280, 0x30);
    imx299_write_register(ViPipe, 0x3281, 0x00);
    imx299_write_register(ViPipe, 0x3284, 0x13);
    imx299_write_register(ViPipe, 0x3285, 0x00);
    imx299_write_register(ViPipe, 0x3286, 0x1F);
    imx299_write_register(ViPipe, 0x3287, 0x00);
    imx299_write_register(ViPipe, 0x3288, 0x30);
    imx299_write_register(ViPipe, 0x3289, 0x00);
    imx299_write_register(ViPipe, 0x328C, 0x13);
    imx299_write_register(ViPipe, 0x328D, 0x00);
    imx299_write_register(ViPipe, 0x328E, 0x1F);
    imx299_write_register(ViPipe, 0x328F, 0x00);
    imx299_write_register(ViPipe, 0x3290, 0x30);
    imx299_write_register(ViPipe, 0x3291, 0x00);
    imx299_write_register(ViPipe, 0x32AE, 0x00);
    imx299_write_register(ViPipe, 0x32AF, 0x00);
    imx299_write_register(ViPipe, 0x32CA, 0x5A);
    imx299_write_register(ViPipe, 0x32CB, 0x00);
    imx299_write_register(ViPipe, 0x332F, 0x00);
    imx299_write_register(ViPipe, 0x334C, 0x01);
    imx299_write_register(ViPipe, 0x335A, 0x79);
    imx299_write_register(ViPipe, 0x335B, 0x00);
    imx299_write_register(ViPipe, 0x335E, 0x56);
    imx299_write_register(ViPipe, 0x335F, 0x00);
    imx299_write_register(ViPipe, 0x3360, 0x6A);
    imx299_write_register(ViPipe, 0x3361, 0x00);
    imx299_write_register(ViPipe, 0x336A, 0x56);
    imx299_write_register(ViPipe, 0x336B, 0x00);
    imx299_write_register(ViPipe, 0x33D6, 0x79);
    imx299_write_register(ViPipe, 0x33D7, 0x00);
    imx299_write_register(ViPipe, 0x340C, 0x6E);
    imx299_write_register(ViPipe, 0x340D, 0x00);
    imx299_write_register(ViPipe, 0x3448, 0x7E);
    imx299_write_register(ViPipe, 0x3449, 0x00);
    imx299_write_register(ViPipe, 0x348E, 0x6F);
    imx299_write_register(ViPipe, 0x348F, 0x00);
    imx299_write_register(ViPipe, 0x3492, 0x11);
    imx299_write_register(ViPipe, 0x34C4, 0x5A);
    imx299_write_register(ViPipe, 0x34C5, 0x00);
    imx299_write_register(ViPipe, 0x3506, 0x56);
    imx299_write_register(ViPipe, 0x3507, 0x00);
    imx299_write_register(ViPipe, 0x350C, 0x56);
    imx299_write_register(ViPipe, 0x350D, 0x00);
    imx299_write_register(ViPipe, 0x350E, 0x58);
    imx299_write_register(ViPipe, 0x350F, 0x00);
    imx299_write_register(ViPipe, 0x3549, 0x04);
    imx299_write_register(ViPipe, 0x355D, 0x03);
    imx299_write_register(ViPipe, 0x355E, 0x03);
    imx299_write_register(ViPipe, 0x3574, 0x56);
    imx299_write_register(ViPipe, 0x3575, 0x00);
    imx299_write_register(ViPipe, 0x3587, 0x01);
    imx299_write_register(ViPipe, 0x35D0, 0x5E);
    imx299_write_register(ViPipe, 0x35D1, 0x00);
    imx299_write_register(ViPipe, 0x35D4, 0x63);
    imx299_write_register(ViPipe, 0x35D5, 0x00);
    imx299_write_register(ViPipe, 0x366A, 0x1A);
    imx299_write_register(ViPipe, 0x366B, 0x16);
    imx299_write_register(ViPipe, 0x366C, 0x10);
    imx299_write_register(ViPipe, 0x366D, 0x09);
    imx299_write_register(ViPipe, 0x366E, 0x00);
    imx299_write_register(ViPipe, 0x366F, 0x00);
    imx299_write_register(ViPipe, 0x3670, 0x00);
    imx299_write_register(ViPipe, 0x3671, 0x00);
    imx299_write_register(ViPipe, 0x3676, 0x83);
    imx299_write_register(ViPipe, 0x3677, 0x03);
    imx299_write_register(ViPipe, 0x3678, 0x00);
    imx299_write_register(ViPipe, 0x3679, 0x04);
    imx299_write_register(ViPipe, 0x367A, 0x2C);
    imx299_write_register(ViPipe, 0x367B, 0x05);
    imx299_write_register(ViPipe, 0x367C, 0x00);
    imx299_write_register(ViPipe, 0x367D, 0x06);
    imx299_write_register(ViPipe, 0x367E, 0x00);
    imx299_write_register(ViPipe, 0x367F, 0x07);
    imx299_write_register(ViPipe, 0x3680, 0x4B);
    imx299_write_register(ViPipe, 0x3681, 0x07);
    imx299_write_register(ViPipe, 0x3690, 0x27);
    imx299_write_register(ViPipe, 0x3691, 0x00);
    imx299_write_register(ViPipe, 0x3692, 0x65);
    imx299_write_register(ViPipe, 0x3693, 0x00);
    imx299_write_register(ViPipe, 0x3694, 0x4F);
    imx299_write_register(ViPipe, 0x3695, 0x00);
    imx299_write_register(ViPipe, 0x3696, 0xA1);
    imx299_write_register(ViPipe, 0x3697, 0x00);
    imx299_write_register(ViPipe, 0x382B, 0x68);
    imx299_write_register(ViPipe, 0x3C00, 0x01);
    imx299_write_register(ViPipe, 0x3C01, 0x01);
    imx299_write_register(ViPipe, 0x3686, 0x00);
    imx299_write_register(ViPipe, 0x3687, 0x00);
    imx299_write_register(ViPipe, 0x36BE, 0x01);
    imx299_write_register(ViPipe, 0x36BF, 0x00);
    imx299_write_register(ViPipe, 0x36C0, 0x01);
    imx299_write_register(ViPipe, 0x36C1, 0x00);
    imx299_write_register(ViPipe, 0x36C2, 0x01);
    imx299_write_register(ViPipe, 0x36C3, 0x00);
    imx299_write_register(ViPipe, 0x3004, 0x04);
    imx299_write_register(ViPipe, 0x3005, 0x01);
    imx299_write_register(ViPipe, 0x3006, 0x01);
    imx299_write_register(ViPipe, 0x3007, 0xA0);
    imx299_write_register(ViPipe, 0x300E, 0x00);
    imx299_write_register(ViPipe, 0x300F, 0x00);
    imx299_write_register(ViPipe, 0x3018, 0x02);
    imx299_write_register(ViPipe, 0x3019, 0x00);
    imx299_write_register(ViPipe, 0x302C, 0x05);
    imx299_write_register(ViPipe, 0x302D, 0x00);
    imx299_write_register(ViPipe, 0x302E, 0x05);
    imx299_write_register(ViPipe, 0x302F, 0x00);
    imx299_write_register(ViPipe, 0x3030, 0x77);
    imx299_write_register(ViPipe, 0x3034, 0x00);
    imx299_write_register(ViPipe, 0x3035, 0x01);
    imx299_write_register(ViPipe, 0x3036, 0x30);
    imx299_write_register(ViPipe, 0x3037, 0x00);
    imx299_write_register(ViPipe, 0x3038, 0x50);
    imx299_write_register(ViPipe, 0x3039, 0x0f);
    imx299_write_register(ViPipe, 0x3068, 0x44);
    imx299_write_register(ViPipe, 0x3069, 0x00);
    imx299_write_register(ViPipe, 0x3080, 0x00);
    imx299_write_register(ViPipe, 0x3081, 0x01);
    imx299_write_register(ViPipe, 0x3084, 0x20);
    imx299_write_register(ViPipe, 0x3085, 0x04);
    imx299_write_register(ViPipe, 0x3086, 0x20);
    imx299_write_register(ViPipe, 0x3087, 0x04);
    imx299_write_register(ViPipe, 0x30A8, 0x02);
    imx299_write_register(ViPipe, 0x30a9, 0xe3);
    imx299_write_register(ViPipe, 0x30aa, 0x08);
    imx299_write_register(ViPipe, 0x30ab, 0x00);
    imx299_write_register(ViPipe, 0x30ac, 0x20);
    imx299_write_register(ViPipe, 0x30ad, 0x04);
    imx299_write_register(ViPipe, 0x30E2, 0x00);
    imx299_write_register(ViPipe, 0x312F, 0x10);
    imx299_write_register(ViPipe, 0x3130, 0x10);
    imx299_write_register(ViPipe, 0x3131, 0x11);
    imx299_write_register(ViPipe, 0x3132, 0x00);
    imx299_write_register(ViPipe, 0x3133, 0x11);
    imx299_write_register(ViPipe, 0x332c, 0x0f);
    imx299_write_register(ViPipe, 0x332d, 0x00);
    imx299_write_register(ViPipe, 0x334a, 0x0f);
    imx299_write_register(ViPipe, 0x334b, 0x00);
    imx299_write_register(ViPipe, 0x357F, 0x0C);
    imx299_write_register(ViPipe, 0x3580, 0x0A);
    imx299_write_register(ViPipe, 0x3581, 0x0a);
    imx299_write_register(ViPipe, 0x3583, 0x75);
    imx299_write_register(ViPipe, 0x35b6, 0x0f);
    imx299_write_register(ViPipe, 0x35b7, 0x00);
    imx299_write_register(ViPipe, 0x35b8, 0x0a);
    imx299_write_register(ViPipe, 0x35b9, 0x00);
    imx299_write_register(ViPipe, 0x36bc, 0x0f);
    imx299_write_register(ViPipe, 0x36bd, 0x00);
    imx299_write_register(ViPipe, 0x3846, 0x00);
    imx299_write_register(ViPipe, 0x3847, 0x00);
    imx299_write_register(ViPipe, 0x384A, 0x00);
    imx299_write_register(ViPipe, 0x384B, 0x00);
    delay_ms(10);
    imx299_write_register(ViPipe, 0x3000, 0x02);
    imx299_write_register(ViPipe, 0x35e5, 0x92);
    imx299_write_register(ViPipe, 0x35e5, 0x9A);
    imx299_write_register(ViPipe, 0x3000, 0x00);
    delay_ms(10);
    imx299_write_register(ViPipe, 0x3033, 0x20);
    imx299_write_register(ViPipe, 0x3017, 0xA8);

    printf("===sony imx299 sensor 3840x2160@30fps 2to1 HDR-1,RAW10(MIPI port) init success!=====\n");

    return;
}



