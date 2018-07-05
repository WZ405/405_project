#!/bin/sh

# This is a sample, you should rewrite it according to your chip #
# clock will be closed at uboot, so you needn't call it!

clk_close()
{
    # Below clock operation is all from Hi3559A, you should modify by datasheet!
    echo "clock close operation done!"
}

# open module clock while you need it!
clk_cfg()
{
    himm 0x12010104 0x006db6db  #MIPI_RX 600M
    himm 0x12010108 0x006db6db  #ISP 600M
    himm 0x1201010c 0x000db030  #(VI_PPC VIPROC0/offline) 600M
    himm 0x12010100 0x00000000  #VICAP VIPROC ISP LOW-POWER clock

    himm 0x1201011c 0x00002017
    himm 0x1201011c 0x00000011  # AIAO(50M 1188M)and MIPITX clk and reset(mipitx_ref_cksel)

    himm 0x12010120 0x2815e4c3  # AIAO&LCD clk and reset(lcd_mclk_div)

    himm 0x12010164 0x00fdbfff  #(VEDU 710M; JPGE 750M;  VPSS GDC AVS 600M;  VGS 631M)
    himm 0x12010168 0x00000005  #VPSS offline(IVE 750M;VDH 568M  GME 600M)


    echo "clock configure operation done!"
}


pwr_en()
{
    himm 0x120300ac 0x33333332  # set MISC_CTRL43, NNIE 0/1, GDC1/VGS1, DSP 0/1/2/3, GPU power up
    himm 0x120300ac 0x22222222  # set MISC_CTRL43, NNIE 0/1, GDC1/VGS1, DSP 0/1/2/3, GPU power up
    himm 0x120300b0 0x00033333  # set MISC_CTRL44, AVSP VDH VEDU2 VPSS1 VIPROC1 power up
    himm 0x120300b0 0x00022222  # set MISC_CTRL44, AVSP VDH VEDU2 VPSS1 VIPROC1 power up
}

#clk_close
clk_cfg
pwr_en
