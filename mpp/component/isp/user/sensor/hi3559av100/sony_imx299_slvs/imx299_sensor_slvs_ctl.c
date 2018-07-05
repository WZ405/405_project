/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx299_sensor_slvs_ctl.c
  Version       : Initial Draft
  Author        : Hisilicon BVT ISP group
  Created       : 2014/05/22
  Description   : Sony IMX299 sensor driver
  History       :
  1.Date        : 2014/05/22
  Author        :
  Modification  : Created file

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_sys.h"

#include "hi_spi.h"
#include "imx299_slave_priv.h"
#include "mpi_isp.h"

extern const IMX299_VIDEO_MODE_TBL_S g_astImx299ModeTbl[];
extern ISP_SLAVE_SNS_SYNC_S gstImx299Sync[];

static int g_fd[ISP_MAX_PIPE_NUM] = { -1, -1, -1, -1, -1, -1, -1, -1 };
extern HI_S32 g_Imx299SlaveBindDev[];
extern ISP_SNS_STATE_S       g_astImx299Slvs[ISP_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U     g_aunImx299SlvsBusInfo[];

void imx299_sensor_linear_init(VI_PIPE ViPipe);
void imx299_sensor_wdr_init(VI_PIPE ViPipe);
void imx299_sensor_common_init(VI_PIPE ViPipe);

int imx299_slvs_spi_init(VI_PIPE ViPipe)
{
    char acDevFile[16];
    HI_U8 u8DevNum, u8Cs;

    if (g_fd[ViPipe] >= 0)
    {
        return 0;
    }
    unsigned int value;
    int ret = 0;

    u8DevNum = g_aunImx299SlvsBusInfo[ViPipe].s8SspDev.bit4SspDev;
    u8Cs = g_aunImx299SlvsBusInfo[ViPipe].s8SspDev.bit4SspCs;
    snprintf(acDevFile, sizeof(acDevFile), "/dev/spidev%d.%d", u8DevNum, u8Cs);

    g_fd[ViPipe] = open(acDevFile, 0);
    if (g_fd[ViPipe] < 0)
    {
        printf("Open %s error!\n", acDevFile);
        return -1;
    }

    value = SPI_MODE_3 | SPI_LSB_FIRST;// | SPI_LOOP;
    ret = ioctl(g_fd[ViPipe], SPI_IOC_WR_MODE, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_WR_MODE err, value = %d ret = %d\n", value, ret);
        return ret;
    }

    value = 8;
    ret = ioctl(g_fd[ViPipe], SPI_IOC_WR_BITS_PER_WORD, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_WR_BITS_PER_WORD err, value = %d ret = %d\n", value, ret);
        return ret;
    }

    value = 2000000;
    ret = ioctl(g_fd[ViPipe], SPI_IOC_WR_MAX_SPEED_HZ, &value);
    if (ret < 0)
    {
        printf("ioctl SPI_IOC_WR_MAX_SPEED_HZ err, value = %d ret = %d\n", value, ret);
        return ret;
    }

    return 0;
}

int imx299_slvs_spi_exit(VI_PIPE ViPipe)
{
    if (g_fd[ViPipe] >= 0)
    {
        close(g_fd[ViPipe]);
        g_fd[ViPipe] = -1;
        return 0;
    }
    return -1;
}

int imx299_slvs_write_register(VI_PIPE ViPipe, unsigned int addr, unsigned char data)
{
    int ret;
    struct spi_ioc_transfer mesg[1];
    unsigned char  tx_buf[8] = {0};
    unsigned char  rx_buf[8] = {0};

    if (0 > g_fd[ViPipe])
    {
        return 0;
    }

    tx_buf[0] = 0x81;
    tx_buf[1] = addr >> 8;
    tx_buf[2] = addr & 0xff;
    tx_buf[3] = data;

    memset(mesg, 0, sizeof(mesg));
    mesg[0].tx_buf = (HI_U64)tx_buf;
    mesg[0].len    = 4;
    mesg[0].rx_buf = (HI_U64)rx_buf;
    mesg[0].cs_change = 1;


    ret = ioctl(g_fd[ViPipe], SPI_IOC_MESSAGE(1), mesg);
    if (ret < 0)
    {
        printf("WriteReg: SPI_IOC_MESSAGE error \n");
        return -1;
    }

    return 0;
}

int imx299_slvs_read_register(VI_PIPE ViPipe, unsigned int addr)
{
    int ret = 0;
    struct spi_ioc_transfer mesg[1];
    unsigned char  tx_buf[8] = {0};
    unsigned char  rx_buf[8] = {0};


    tx_buf[0] = 0x80;
    tx_buf[1] = addr >> 8;
    tx_buf[2] = addr & 0xff;
    tx_buf[3] = 0;

    memset(mesg, 0, sizeof(mesg));
    mesg[0].tx_buf = (HI_U64)tx_buf;
    mesg[0].len    = 4;
    mesg[0].rx_buf = (HI_U64)rx_buf;
    mesg[0].cs_change = 1;

    ret = ioctl(g_fd[ViPipe], SPI_IOC_MESSAGE(1), mesg);
    if (ret  < 0)
    {
        printf("ReadReg: SPI_IOC_MESSAGE error \n");
        return -1;
    }

    return rx_buf[3];
}

static void delay_ms(int ms)
{
    usleep(ms * 1000);
}

void imx299_slvs_standby(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

void imx299_slvs_restart(VI_PIPE ViPipe)
{
    // TODO:
    return;
}

void imx299_slvs_init(VI_PIPE ViPipe)
{
    HI_U8         u8ImgMode;
    HI_S32        SlaveDev;
    u8ImgMode   = g_astImx299Slvs[ViPipe].u8ImgMode;
    SlaveDev    = g_Imx299SlaveBindDev[ViPipe];

    HI_U32 i;

    /* hold sync signal as fixed */
    CHECK_RET(HI_MPI_ISP_GetSnsSlaveAttr(SlaveDev, &gstImx299Sync[ViPipe]));
    gstImx299Sync[ViPipe].unCfg.stBits.bitHEnable = 0;
    gstImx299Sync[ViPipe].unCfg.stBits.bitVEnable = 0;
    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(SlaveDev, &gstImx299Sync[ViPipe]));

    /* 2. imx299 spi init */
    imx299_slvs_spi_init(ViPipe);

    imx299_sensor_common_init(ViPipe);

    /* When sensor first init, config all registers */
    // release hv sync
    gstImx299Sync[ViPipe].u32HsTime = g_astImx299ModeTbl[u8ImgMode].u32InckPerHs;
    if (g_astImx299Slvs[ViPipe].astRegsInfo[0].stSlvSync.u32SlaveVsTime == 0)
    {
        gstImx299Sync[ViPipe].u32VsTime = g_astImx299ModeTbl[u8ImgMode].u32InckPerVs;
    }
    else
    {
        gstImx299Sync[ViPipe].u32VsTime = g_astImx299Slvs[ViPipe].astRegsInfo[0].stSlvSync.u32SlaveVsTime;
    }
    gstImx299Sync[ViPipe].unCfg.u32Bytes = 0xc0030000;
    gstImx299Sync[ViPipe].u32HsCyc = 0x3;
    gstImx299Sync[ViPipe].u32VsCyc = 0x3;
    CHECK_RET(HI_MPI_ISP_SetSnsSlaveAttr(SlaveDev, &gstImx299Sync[ViPipe]));

    // common part of init seq
    if (IMX299_4K120FPS_MODE == u8ImgMode)
    {
        imx299_sensor_linear_init(ViPipe);
    }
    else if (IMX299_4K60FPS_WDR_MODE == u8ImgMode)
    {
        imx299_sensor_wdr_init(ViPipe);
    }
    else
    {}

    for (i = 0; i < g_astImx299Slvs[ViPipe].astRegsInfo[0].u32RegNum; i++)
    {
        imx299_slvs_write_register(ViPipe, g_astImx299Slvs[ViPipe].astRegsInfo[0].astSspData[i].u32RegAddr, g_astImx299Slvs[ViPipe].astRegsInfo[0].astSspData[i].u32Data);
    }

    printf("IMX299 %s init succuss!\n", g_astImx299ModeTbl[u8ImgMode].pszModeName);
    g_astImx299Slvs[ViPipe].bInit = HI_TRUE;

    return ;
}

void imx299_slvs_exit(VI_PIPE ViPipe)
{
    imx299_slvs_spi_exit(ViPipe);

    return;
}


void imx299_sensor_common_init(VI_PIPE ViPipe)
{

    imx299_slvs_write_register (ViPipe, 0x0033 , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x003C , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x00EF , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x01E8 , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x01E9 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0122 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0129 , 0x0C );
    imx299_slvs_write_register (ViPipe, 0x012A , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x011F , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0123 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0124 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0125 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0127 , 0x06 );
    imx299_slvs_write_register (ViPipe, 0x012D , 0x03 );
    imx299_slvs_write_register (ViPipe, 0x0000 , 0x1A );
    imx299_slvs_write_register (ViPipe, 0x010B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0047 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x004E , 0x0B );
    imx299_slvs_write_register (ViPipe, 0x004F , 0x24 );
    imx299_slvs_write_register (ViPipe, 0x0062 , 0x25 );
    imx299_slvs_write_register (ViPipe, 0x0064 , 0x78 );
    imx299_slvs_write_register (ViPipe, 0x0065 , 0x33 );
    imx299_slvs_write_register (ViPipe, 0x0067 , 0x71 );
    imx299_slvs_write_register (ViPipe, 0x0088 , 0x75 );
    imx299_slvs_write_register (ViPipe, 0x008A , 0x09 );
    imx299_slvs_write_register (ViPipe, 0x008B , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x008C , 0x61 );
    imx299_slvs_write_register (ViPipe, 0x0146 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0234 , 0x32 );
    imx299_slvs_write_register (ViPipe, 0x0235 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0248 , 0xBC );
    imx299_slvs_write_register (ViPipe, 0x0249 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0250 , 0xBC );
    imx299_slvs_write_register (ViPipe, 0x0251 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0258 , 0xBC );
    imx299_slvs_write_register (ViPipe, 0x0259 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0260 , 0xBC );
    imx299_slvs_write_register (ViPipe, 0x0261 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0274 , 0x13 );
    imx299_slvs_write_register (ViPipe, 0x0275 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0276 , 0x1F );
    imx299_slvs_write_register (ViPipe, 0x0277 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0278 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x0279 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x027C , 0x13 );
    imx299_slvs_write_register (ViPipe, 0x027D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x027E , 0x1F );
    imx299_slvs_write_register (ViPipe, 0x027F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0280 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x0281 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0284 , 0x13 );
    imx299_slvs_write_register (ViPipe, 0x0285 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0286 , 0x1F );
    imx299_slvs_write_register (ViPipe, 0x0287 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0288 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x0289 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x028C , 0x13 );
    imx299_slvs_write_register (ViPipe, 0x028D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x028E , 0x1F );
    imx299_slvs_write_register (ViPipe, 0x028F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0290 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x0291 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x02AE , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x02AF , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x02CA , 0x5A );
    imx299_slvs_write_register (ViPipe, 0x02CB , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x032F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x034C , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x035A , 0x79 );
    imx299_slvs_write_register (ViPipe, 0x035B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x035E , 0x56 );
    imx299_slvs_write_register (ViPipe, 0x035F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0360 , 0x6A );
    imx299_slvs_write_register (ViPipe, 0x0361 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x036A , 0x56 );
    imx299_slvs_write_register (ViPipe, 0x036B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x03D6 , 0x79 );
    imx299_slvs_write_register (ViPipe, 0x03D7 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x040C , 0x6E );
    imx299_slvs_write_register (ViPipe, 0x040D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0448 , 0x7E );
    imx299_slvs_write_register (ViPipe, 0x0449 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x048E , 0x6F );
    imx299_slvs_write_register (ViPipe, 0x048F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0492 , 0x11 );
    imx299_slvs_write_register (ViPipe, 0x04C4 , 0x5A );
    imx299_slvs_write_register (ViPipe, 0x04C5 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0506 , 0x56 );
    imx299_slvs_write_register (ViPipe, 0x0507 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x050C , 0x56 );
    imx299_slvs_write_register (ViPipe, 0x050D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x050E , 0x58 );
    imx299_slvs_write_register (ViPipe, 0x050F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0549 , 0x04 );
    imx299_slvs_write_register (ViPipe, 0x055D , 0x03 );
    imx299_slvs_write_register (ViPipe, 0x055E , 0x03 );
    imx299_slvs_write_register (ViPipe, 0x0574 , 0x56 );
    imx299_slvs_write_register (ViPipe, 0x0575 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0587 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x05D0 , 0x5E );
    imx299_slvs_write_register (ViPipe, 0x05D1 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x05D4 , 0x63 );
    imx299_slvs_write_register (ViPipe, 0x05D5 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x066A , 0x1A );
    imx299_slvs_write_register (ViPipe, 0x066B , 0x16 );
    imx299_slvs_write_register (ViPipe, 0x066C , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x066D , 0x09 );
    imx299_slvs_write_register (ViPipe, 0x066E , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x066F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0670 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0671 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0676 , 0x83 );
    imx299_slvs_write_register (ViPipe, 0x0677 , 0x03 );
    imx299_slvs_write_register (ViPipe, 0x0678 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0679 , 0x04 );
    imx299_slvs_write_register (ViPipe, 0x067A , 0x2C );
    imx299_slvs_write_register (ViPipe, 0x067B , 0x05 );
    imx299_slvs_write_register (ViPipe, 0x067C , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x067D , 0x06 );
    imx299_slvs_write_register (ViPipe, 0x067E , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x067F , 0x07 );
    imx299_slvs_write_register (ViPipe, 0x0680 , 0x4B );
    imx299_slvs_write_register (ViPipe, 0x0681 , 0x07 );
    imx299_slvs_write_register (ViPipe, 0x0690 , 0x27 );
    imx299_slvs_write_register (ViPipe, 0x0691 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0692 , 0x65 );
    imx299_slvs_write_register (ViPipe, 0x0693 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0694 , 0x4F );
    imx299_slvs_write_register (ViPipe, 0x0695 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0696 , 0xA1 );
    imx299_slvs_write_register (ViPipe, 0x0697 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x082B , 0x68 );
    imx299_slvs_write_register (ViPipe, 0x0C00 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0C01 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0686 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0687 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x06BE , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x06BF , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x06C0 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x06C1 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x06C2 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x06C3 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x06C4 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x06C5 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x06C6 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0008 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x000A , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x000B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0012 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x00F4 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x00F5 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0042 , 0x32 );
    imx299_slvs_write_register (ViPipe, 0x0002 , 0x00 );
    delay_ms(100);
    return;
}

void imx299_sensor_linear_init(VI_PIPE ViPipe)
{
    imx299_slvs_write_register (ViPipe, 0x0000 , 0x0A );
    imx299_slvs_write_register (ViPipe, 0x05E5 , 0x92 );
    imx299_slvs_write_register (ViPipe, 0x05E5 , 0x9A );
    imx299_slvs_write_register (ViPipe, 0x0000 , 0x08 );
    delay_ms(100);
    imx299_slvs_write_register (ViPipe, 0x0001 , 0x11 );
    imx299_slvs_write_register (ViPipe, 0x0003 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0004 , 0x1A );
    imx299_slvs_write_register (ViPipe, 0x0005 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0006 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0007 , 0xA0 );
    imx299_slvs_write_register (ViPipe, 0x000E , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x000F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0019 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x002C , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x002D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x002E , 0x17 );
    imx299_slvs_write_register (ViPipe, 0x002F , 0x08 );
    imx299_slvs_write_register (ViPipe, 0x0030 , 0x77 );
    imx299_slvs_write_register (ViPipe, 0x0034 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0035 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0036 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x0037 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0038 , 0x60 );
    imx299_slvs_write_register (ViPipe, 0x0039 , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x0068 , 0x44 );
    imx299_slvs_write_register (ViPipe, 0x0069 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0080 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0081 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0084 , 0x08 );
    imx299_slvs_write_register (ViPipe, 0x0085 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x0086 , 0x08 );
    imx299_slvs_write_register (ViPipe, 0x0087 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x00A8 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x00E2 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x032C , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x032D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x034A , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x034B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x057F , 0x0C );
    imx299_slvs_write_register (ViPipe, 0x0580 , 0x0A );
    imx299_slvs_write_register (ViPipe, 0x0581 , 0x0A );
    imx299_slvs_write_register (ViPipe, 0x0583 , 0x75 );
    imx299_slvs_write_register (ViPipe, 0x05B6 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x05B7 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x05B8 , 0x2B );
    imx299_slvs_write_register (ViPipe, 0x05B9 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0600 , 0x90 );
    imx299_slvs_write_register (ViPipe, 0x0601 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x06BC , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x06BD , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0846 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0847 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x084A , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x084B , 0x00 );

    return ;
}

void imx299_sensor_wdr_init(VI_PIPE ViPipe)
{
    imx299_slvs_write_register (ViPipe, 0x0000 , 0x0A );
    imx299_slvs_write_register (ViPipe, 0x05E5 , 0x92 );
    imx299_slvs_write_register (ViPipe, 0x05E5 , 0x9A );
    imx299_slvs_write_register (ViPipe, 0x0000 , 0x08 );
    delay_ms(100);
    imx299_slvs_write_register (ViPipe, 0x0001 , 0x11 );
    imx299_slvs_write_register (ViPipe, 0x0003 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0004 , 0x04 );
    imx299_slvs_write_register (ViPipe, 0x0005 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0006 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0007 , 0xA0 );
    imx299_slvs_write_register (ViPipe, 0x000E , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x000F , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0019 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x002C , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x002D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x002E , 0x17 );
    imx299_slvs_write_register (ViPipe, 0x002F , 0x08 );
    imx299_slvs_write_register (ViPipe, 0x0030 , 0x77 );
    imx299_slvs_write_register (ViPipe, 0x0034 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0035 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0036 , 0x30 );
    imx299_slvs_write_register (ViPipe, 0x0037 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0038 , 0x60 );
    imx299_slvs_write_register (ViPipe, 0x0039 , 0x10 );
    imx299_slvs_write_register (ViPipe, 0x0068 , 0x44 );
    imx299_slvs_write_register (ViPipe, 0x0069 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0080 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0081 , 0x01 );
    imx299_slvs_write_register (ViPipe, 0x0084 , 0x08 );
    imx299_slvs_write_register (ViPipe, 0x0085 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x0086 , 0x08 );
    imx299_slvs_write_register (ViPipe, 0x0087 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x00A8 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x00E2 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x032C , 0x68 );
    imx299_slvs_write_register (ViPipe, 0x032D , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x034A , 0x68 );
    imx299_slvs_write_register (ViPipe, 0x034B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x057F , 0x0C );
    imx299_slvs_write_register (ViPipe, 0x0580 , 0x0A );
    imx299_slvs_write_register (ViPipe, 0x0581 , 0x0A );
    imx299_slvs_write_register (ViPipe, 0x0583 , 0x75 );
    imx299_slvs_write_register (ViPipe, 0x05B6 , 0x68 );
    imx299_slvs_write_register (ViPipe, 0x05B7 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x05B8 , 0x63 );
    imx299_slvs_write_register (ViPipe, 0x05B9 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0600 , 0x90 );
    imx299_slvs_write_register (ViPipe, 0x0601 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x06BC , 0x68 );
    imx299_slvs_write_register (ViPipe, 0x06BD , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0846 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0847 , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x084A , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x084B , 0x00 );
    imx299_slvs_write_register (ViPipe, 0x0018 , 0x02 );
    imx299_slvs_write_register (ViPipe, 0x00AE , 0x02 );

    return ;
}

