
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx290_sensor_ctl_slave.c

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

#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"

#include "imx290_slave_priv.h"
#include "mpi_isp.h"


#ifdef HI_GPIO_I2C
#include "gpioi2c_ex.h"
#else
#include "hi_i2c.h"
#endif


extern const IMX290_VIDEO_MODE_TBL_S g_astImx290ModeTbl[];
extern ISP_SLAVE_SNS_SYNC_S gstImx290Sync[];

const unsigned char imx290slave_i2c_addr     =    0x34;        /* I2C Address of IMX290 */
const unsigned int  imx290slave_addr_byte    =    2;
const unsigned int  imx290slave_data_byte    =    1;
static int g_fd[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = -1};

extern HI_S32 g_Imx290SlaveBindDev[];

extern ISP_SNS_STATE_S        g_astImx290Slave[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U      g_aunImx290SlaveBusInfo[];

int imx290slave_i2c_init(VI_PIPE ViPipe)
{
    char acDevFile[16] = {0};
    HI_U8 u8DevNum;

    if (g_fd[ViPipe] >= 0)
    {
        return 0;
    }
#ifdef HI_GPIO_I2C
    int ret;

    g_fd[ViPipe] = open("/dev/gpioi2c_ex", 0);
    if (g_fd[ViPipe] < 0)
    {
        printf("Open gpioi2c_ex error!\n");
        return -1;
    }
#else
    int ret;

    u8DevNum = g_aunImx290SlaveBusInfo[ViPipe].s8I2cDev;
    snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

    g_fd[ViPipe] = open(acDevFile, O_RDWR);

    if (g_fd[ViPipe] < 0)
    {
        printf("Open /dev/hi_i2c_drv-%u error!\n", u8DevNum);
        return -1;
    }

    ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, (imx290slave_i2c_addr >> 1));
    if (ret < 0)
    {
        printf("I2C_SLAVE_FORCE error!\n");
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return ret;
    }
#endif

    return 0;
}

int imx290slave_i2c_exit(VI_PIPE ViPipe)
{
    if (g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return 0;
    }
    return -1;
}

int imx290slave_read_register(VI_PIPE ViPipe, int addr)
{
    // TODO:

    return 0;
}

int imx290slave_write_register(VI_PIPE ViPipe, int addr, int data)
{
    if (0 > g_fd[ViPipe])
    {
        return 0;
    }

#ifdef HI_GPIO_I2C
    i2c_data.dev_addr = imx290slave_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = imx290slave_addr_byte;
    i2c_data.data = data;
    i2c_data.data_byte_num = imx290slave_data_byte;

    ret = ioctl(g_fd[ViPipe], GPIO_I2C_WRITE, &i2c_data);

    if (ret)
    {
        printf("GPIO-I2C write faild!\n");
        return ret;
    }
#else
    int idx = 0;
    int ret;
    char buf[8];

    if (imx290slave_addr_byte == 2)
    {
        buf[idx] = (addr >> 8) & 0xff;
        idx++;
        buf[idx] = addr & 0xff;
        idx++;
    }
    else
    {
        //buf[idx] = addr & 0xff;
        //idx++;
    }

    if (imx290slave_data_byte == 2)
    {
        //buf[idx] = (data >> 8) & 0xff;
        //idx++;
        //buf[idx] = data & 0xff;
        //idx++;
    }
    else
    {
        buf[idx] = data & 0xff;
        idx++;
    }

    ret = write(g_fd[ViPipe], buf, imx290slave_addr_byte + imx290slave_data_byte);
    if (ret < 0)
    {
        printf("I2C_WRITE error!\n");
        return -1;
    }

#endif
    return 0;
}


static void delay_ms(int ms)
{
    usleep(ms * 1000);
}

void imx290slave_prog(VI_PIPE ViPipe, int *rom)
{
    int i = 0;
    while (1)
    {
        int lookup = rom[i++];
        int addr = (lookup >> 16) & 0xFFFF;
        int data = lookup & 0xFFFF;
        if (addr == 0xFFFE)
        {
            delay_ms(data);
        }
        else if (addr == 0xFFFF)
        {
            return;
        }
        else
        {
            imx290slave_write_register(ViPipe, addr, data);
        }
    }
}

void imx290slave_standby(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

void imx290slave_restart(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

#define IMX290_SENSOR_1080P_60FPS_LINEAR_MODE      (1)

void imx290slave_linear_1080p60_init(VI_PIPE ViPipe);

void imx290slave_init(VI_PIPE ViPipe)
{
    HI_U32         i;
    WDR_MODE_E    enWDRMode;
    HI_BOOL       bInit;
    HI_S32        SlaveDev;
    HI_U8         u8ImgMode;

    bInit       = g_astImx290Slave[ViPipe].bInit;
    enWDRMode   = g_astImx290Slave[ViPipe].enWDRMode;
    SlaveDev    = g_Imx290SlaveBindDev[ViPipe];
    u8ImgMode   = g_astImx290Slave[ViPipe].u8ImgMode;

    /* hold sync signal as fixed */
    CHECK_RET(HI_MPI_ISP_GetSnsSlaveAttr(SlaveDev, &gstImx290Sync[ViPipe]));
    gstImx290Sync[ViPipe].unCfg.stBits.bitHEnable = 0;
    gstImx290Sync[ViPipe].unCfg.stBits.bitVEnable = 0;
    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(SlaveDev, &gstImx290Sync[ViPipe]));

    imx290slave_i2c_init(ViPipe);

    CHECK_RET(HI_MPI_ISP_GetSnsSlaveAttr(SlaveDev, &gstImx290Sync[ViPipe]));

    /* When sensor first init, config all registers */
    // release hv sync
    gstImx290Sync[ViPipe].u32HsTime = g_astImx290ModeTbl[u8ImgMode].u32InckPerHs;
    if (g_astImx290Slave[ViPipe].astRegsInfo[0].stSlvSync.u32SlaveVsTime == 0)
    {
        gstImx290Sync[ViPipe].u32VsTime = g_astImx290ModeTbl[u8ImgMode].u32InckPerVs;
    }
    else
    {
        gstImx290Sync[ViPipe].u32VsTime = g_astImx290Slave[ViPipe].astRegsInfo[0].stSlvSync.u32SlaveVsTime;
    }

    //printf("u32HsTime = %d u32VsTime = %d \n",gstImx290Sync[ViPipe].u32HsTime,gstImx290Sync[ViPipe].u32VsTime);
    gstImx290Sync[ViPipe].unCfg.u32Bytes = 0xc0030000;
    gstImx290Sync[ViPipe].u32HsCyc = 0x3;
    gstImx290Sync[ViPipe].u32VsCyc = 0x3;
    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(SlaveDev, &gstImx290Sync[ViPipe]));

    /* When sensor first init, config all registers */
    if (HI_FALSE == bInit)
    {
        if (WDR_MODE_NONE == enWDRMode && IMX290_2M60FPS_LINER_MODE == u8ImgMode)
        {
            imx290slave_linear_1080p60_init(ViPipe);
        }
        else
        {

        }
    }
    /* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
    else
    {
        if (WDR_MODE_NONE == enWDRMode && IMX290_2M60FPS_LINER_MODE == u8ImgMode)
        {
            imx290slave_linear_1080p60_init(ViPipe);
        }
        else
        {

        }
    }

    for (i = 0; i < g_astImx290Slave[ViPipe].astRegsInfo[0].u32RegNum; i++)
    {
        imx290slave_write_register(ViPipe, g_astImx290Slave[ViPipe].astRegsInfo[0].astI2cData[i].u32RegAddr, g_astImx290Slave[ViPipe].astRegsInfo[0].astI2cData[i].u32Data);
    }

    g_astImx290Slave[ViPipe].bInit = HI_TRUE;
    return ;
}

void imx290slave_exit(VI_PIPE ViPipe)
{
    imx290slave_i2c_exit(ViPipe);

    return;
}


/* 1080P60 */
void imx290slave_linear_1080p60_init(VI_PIPE ViPipe)
{
    imx290slave_write_register (ViPipe, 0x3000, 0x01); /* standby */

    imx290slave_write_register (ViPipe, 0x3002, 0x01);
    imx290slave_write_register (ViPipe, 0x3005, 0x00);
    imx290slave_write_register (ViPipe, 0x3007, 0x00);
    imx290slave_write_register (ViPipe, 0x3009, 0x01);
    imx290slave_write_register (ViPipe, 0x300A, 0x3C);
    imx290slave_write_register (ViPipe, 0x300F, 0x00);
    imx290slave_write_register (ViPipe, 0x3010, 0x21);
    imx290slave_write_register (ViPipe, 0x3012, 0x64);
    imx290slave_write_register (ViPipe, 0x3016, 0x09);
    imx290slave_write_register (ViPipe, 0x3018, 0x65);
    imx290slave_write_register (ViPipe, 0x3019, 0x04);
    imx290slave_write_register (ViPipe, 0x301C, 0x98);
    imx290slave_write_register (ViPipe, 0x301D, 0x08);
    imx290slave_write_register (ViPipe, 0x3046, 0x00);
    imx290slave_write_register (ViPipe, 0x304B, 0x0A);
    imx290slave_write_register (ViPipe, 0x305C, 0x0C);
    imx290slave_write_register (ViPipe, 0x305D, 0x03);
    imx290slave_write_register (ViPipe, 0x305E, 0x10);
    imx290slave_write_register (ViPipe, 0x305F, 0x01);
    imx290slave_write_register (ViPipe, 0x3070, 0x02);
    imx290slave_write_register (ViPipe, 0x3071, 0x11);
    imx290slave_write_register (ViPipe, 0x309B, 0x10);
    imx290slave_write_register (ViPipe, 0x309C, 0x22);
    imx290slave_write_register (ViPipe, 0x30A2, 0x02);
    imx290slave_write_register (ViPipe, 0x30A6, 0x20);
    imx290slave_write_register (ViPipe, 0x30A8, 0x20);
    imx290slave_write_register (ViPipe, 0x30AA, 0x20);
    imx290slave_write_register (ViPipe, 0x30AC, 0x20);
    imx290slave_write_register (ViPipe, 0x30B0, 0x43);

    imx290slave_write_register (ViPipe, 0x3119, 0x9E);
    imx290slave_write_register (ViPipe, 0x311C, 0x1E);
    imx290slave_write_register (ViPipe, 0x311E, 0x08);
    imx290slave_write_register (ViPipe, 0x3128, 0x05);
    imx290slave_write_register (ViPipe, 0x3129, 0x1D);
    imx290slave_write_register (ViPipe, 0x313D, 0x83);
    imx290slave_write_register (ViPipe, 0x3150, 0x03);
    imx290slave_write_register (ViPipe, 0x315E, 0x1B);
    imx290slave_write_register (ViPipe, 0x3164, 0x1B);
    imx290slave_write_register (ViPipe, 0x317C, 0x12);
    imx290slave_write_register (ViPipe, 0x317E, 0x00);
    imx290slave_write_register (ViPipe, 0x31EC, 0x37);

    imx290slave_write_register (ViPipe, 0x32B8, 0x50);
    imx290slave_write_register (ViPipe, 0x32B9, 0x10);
    imx290slave_write_register (ViPipe, 0x32BA, 0x00);
    imx290slave_write_register (ViPipe, 0x32BB, 0x04);
    imx290slave_write_register (ViPipe, 0x32C8, 0x50);
    imx290slave_write_register (ViPipe, 0x32C9, 0x10);
    imx290slave_write_register (ViPipe, 0x32CA, 0x00);
    imx290slave_write_register (ViPipe, 0x32CB, 0x04);

    imx290slave_write_register (ViPipe, 0x332C, 0xD3);
    imx290slave_write_register (ViPipe, 0x332D, 0x10);
    imx290slave_write_register (ViPipe, 0x332E, 0x0D);
    imx290slave_write_register (ViPipe, 0x3358, 0x06);
    imx290slave_write_register (ViPipe, 0x3359, 0xE1);
    imx290slave_write_register (ViPipe, 0x335A, 0x11);
    imx290slave_write_register (ViPipe, 0x3360, 0x1E);
    imx290slave_write_register (ViPipe, 0x3361, 0x61);
    imx290slave_write_register (ViPipe, 0x3362, 0x10);
    imx290slave_write_register (ViPipe, 0x33B0, 0x50);
    imx290slave_write_register (ViPipe, 0x33B2, 0x1A);
    imx290slave_write_register (ViPipe, 0x33B3, 0x04);

    imx290slave_write_register (ViPipe, 0x3405, 0x00);
    imx290slave_write_register (ViPipe, 0x3407, 0x01);
    imx290slave_write_register (ViPipe, 0x3414, 0x0A);
    imx290slave_write_register (ViPipe, 0x3418, 0x49);
    imx290slave_write_register (ViPipe, 0x3419, 0x04);
    imx290slave_write_register (ViPipe, 0x3441, 0x0A);
    imx290slave_write_register (ViPipe, 0x3442, 0x0A);
    imx290slave_write_register (ViPipe, 0x3443, 0x01);
    imx290slave_write_register (ViPipe, 0x3444, 0x40);
    imx290slave_write_register (ViPipe, 0x3445, 0x4A);
    imx290slave_write_register (ViPipe, 0x3446, 0x77);
    imx290slave_write_register (ViPipe, 0x3447, 0x00);
    imx290slave_write_register (ViPipe, 0x3448, 0x67);
    imx290slave_write_register (ViPipe, 0x3449, 0x00);
    imx290slave_write_register (ViPipe, 0x344A, 0x47);
    imx290slave_write_register (ViPipe, 0x344B, 0x00);
    imx290slave_write_register (ViPipe, 0x344C, 0x37);
    imx290slave_write_register (ViPipe, 0x344D, 0x00);
    imx290slave_write_register (ViPipe, 0x344E, 0x3F);
    imx290slave_write_register (ViPipe, 0x344F, 0x00);
    imx290slave_write_register (ViPipe, 0x3450, 0xFF);
    imx290slave_write_register (ViPipe, 0x3451, 0x00);
    imx290slave_write_register (ViPipe, 0x3452, 0x3F);
    imx290slave_write_register (ViPipe, 0x3453, 0x00);
    imx290slave_write_register (ViPipe, 0x3454, 0x37);
    imx290slave_write_register (ViPipe, 0x3455, 0x00);
    imx290slave_write_register (ViPipe, 0x3472, 0x9C);
    imx290slave_write_register (ViPipe, 0x3473, 0x07);
    imx290slave_write_register (ViPipe, 0x3480, 0x92);
    imx290slave_write_register (ViPipe, 0x3000, 0x00); /* standby */
    delay_ms(20);

    printf("===IMX290 1080P 60fps 10bit Slave Mode LINE Init OK!===\n");

    return;
}


