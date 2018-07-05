#!/bin/sh

# This is a sample, you should rewrite it according to your chip #

# mddrc pri&timeout setting
### pri 0~7,0 lowest, 7 highest
#########################################################################################
# param $1=1 --- online
# param $1=0 --- offline
vi_vpss_online_config()
{
	# -------------vi vpss online open
	if [ $b_vpss_online -eq 1 ]; then
		echo "==============vi_vcap_online==============";  
		                               #each module 3bit:           30:28           26:24           22:20          18:16        14:12         10:8         6:4         2:0
		himm 0x12032000  0x00000011    #each module 3bit:          SPACC             FMC           EDMA0          EDMA1          UFS         EMMC         GSF1         GSF0 
		himm 0x12032004  0x00454545    #each module 3bit:                                         vedu2(W)        vedu2(R)     vedu1(W)     vedu1(R)     vedu0(W)     vedu0(R)
		himm 0x12032008  0x34344545    #each module 3bit:         vgs1(W)         vgs1(R)          vgs0(W)         vgs0(R)     vpss1(W)     vpss1(R)    vpss0 (W)     vpss0(R)   
		himm 0x1203200c  0x55554545    #each module 3bit:         gdc1(W)         gdc1(R)          gdc0(W)         gdc0(R)     jpge1(W)     jpge1(R)     jpge0(W)     jpge0(R)
		himm 0x12032010  0x55003333    #each module 3bit:          gme(W)          gme(R)          gzip(W)         gzip(R)      jpgd(W)      jpgd(R)       pgd(W)       pgd(R)  
		himm 0x12032014  0x67676767    #each module 3bit:      viproc1(W)      viproc1(R)       viproc0(W)      viproc0(R)     vi_m1(W)     vi_m1(R)     vi_m0(W)     vi_m0(R)
		himm 0x12032018  0x33666767    #each module 3bit:          gpu(W)          gpu(R)           aio(W)          aio(R)      vdp1(W)      vdp1(R)      vdp0(W)      vdp0(R)  
		himm 0x1203201c  0x00000000    #each module 3bit:                                         sdio2(W)        sdio2(R)     sdio1(W)     sdio1(R)     sdio0(W)     sdio0(R)
		himm 0x12032020  0x12333333    #each module 3bit:           fd(W)           fd(R)           dpu(W)          dpu(R)      cnn1(W)      cnn1(R)      cnn0(W)      cnn0(R)
		himm 0x12032024  0x33333333    #each module 3bit:    dsp1_idma(W)    dsp1_idma(R)        dsp1_m(W)       dsp1_m(R) dsp0_idma(W) dsp0_idma(R)    dsp0_m(W)    dsp0_m(R)  
		himm 0x12032028  0x33333333    #each module 3bit:    dsp3_idma(W)    dsp3_idma(R)        dsp3_m(W)       dsp3_m(R) dsp2_idma(W) dsp2_idma(R)    dsp2_m(W)    dsp2_m(R)
		himm 0x1203202c  0x00444444    #each module 3bit:         pcie(W)         pcie(R)        avs_m2(W)       avs_m2(R)    avs_m1(W)    avs_m1(R)    avs_m0(W)    avs_m0(R)
		himm 0x12032030  0x55552211    #each module 3bit:         peri(W)         peri(R)           a53(W)          a53(R)       ive(W)       ive(R)       TDE(W)      TDE(R)
		himm 0x12032034  0x55550000    #each module 3bit:         cpu1(W)         cpu1(R)          cpu0(W)         cpu0(R)     ddrt1(W)     ddrt1(R)     ddrt0(W)     ddrt0(R)    
		himm 0x12032038  0x77776666    #each module 3bit:       usb3p1(W)       usb3p1(R)        usb3p0(W)       usb3p0(R)      vdh1(W)      vdh1(R)      vdh0(W)      vdh0(R)
	else
		echo "==============vi_vcap_offline==============";
                                       #each module 3bit:           30:28           26:24           22:20          18:16        14:12         10:8         6:4         2:0
		himm 0x12032000  0x00000011    #each module 3bit:          SPACC             FMC           EDMA0          EDMA1          UFS         EMMC         GSF1         GSF0 
		himm 0x12032004  0x00454545    #each module 3bit:                                         vedu2(W)        vedu2(R)     vedu1(W)     vedu1(R)     vedu0(W)     vedu0(R)
		himm 0x12032008  0x44444545    #each module 3bit:         vgs1(W)         vgs1(R)          vgs0(W)         vgs0(R)     vpss1(W)     vpss1(R)    vpss0 (W)     vpss0(R)   
		himm 0x1203200c  0x55554545    #each module 3bit:         gdc1(W)         gdc1(R)          gdc0(W)         gdc0(R)     jpge1(W)     jpge1(R)     jpge0(W)     jpge0(R)
		himm 0x12032010  0x55003333    #each module 3bit:          gme(W)          gme(R)          gzip(W)         gzip(R)      jpgd(W)      jpgd(R)       pgd(W)       pgd(R)  
		himm 0x12032014  0x67677777    #each module 3bit:      viproc1(W)      viproc1(R)       viproc0(W)      viproc0(R)     vi_m1(W)     vi_m1(R)     vi_m0(W)     vi_m0(R)
		himm 0x12032018  0x33666767    #each module 3bit:          gpu(W)          gpu(R)           aio(W)          aio(R)      vdp1(W)      vdp1(R)      vdp0(W)      vdp0(R)  
		himm 0x1203201c  0x00000000    #each module 3bit:                                         sdio2(W)        sdio2(R)     sdio1(W)     sdio1(R)     sdio0(W)     sdio0(R)
		himm 0x12032020  0x11333333    #each module 3bit:           fd(W)           fd(R)           dpu(W)          dpu(R)      cnn1(W)      cnn1(R)      cnn0(W)      cnn0(R)
		himm 0x12032024  0x33333333    #each module 3bit:    dsp1_idma(W)    dsp1_idma(R)        dsp1_m(W)       dsp1_m(R) dsp0_idma(W) dsp0_idma(R)    dsp0_m(W)    dsp0_m(R)  
		himm 0x12032028  0x33333333    #each module 3bit:    dsp3_idma(W)    dsp3_idma(R)        dsp3_m(W)       dsp3_m(R) dsp2_idma(W) dsp2_idma(R)    dsp2_m(W)    dsp2_m(R)
		himm 0x1203202c  0x00444444    #each module 3bit:         pcie(W)         pcie(R)        avs_m2(W)       avs_m2(R)    avs_m1(W)    avs_m1(R)    avs_m0(W)    avs_m0(R)
		himm 0x12032030  0x55552211    #each module 3bit:         peri(W)         peri(R)           a53(W)          a53(R)       ive(W)       ive(R)       TDE(W)      TDE(R)
		himm 0x12032034  0x55550000    #each module 3bit:         cpu1(W)         cpu1(R)          cpu0(W)         cpu0(R)     ddrt1(W)     ddrt1(R)     ddrt0(W)     ddrt0(R)    
		himm 0x12032038  0x77776666    #each module 3bit:       usb3p1(W)       usb3p1(R)        usb3p0(W)       usb3p0(R)      vdh1(W)      vdh1(R)      vdh0(W)      vdh0(R)
	fi  
}
#########################################################################################

############## amp unmute ##########
himm 0x1F002050 0x00000cf0 
himm 0x1214E200 0x80
himm 0x1214E400 0x80
himm 0x1214E200 0x80

##############  qosbuf #############
himm  0x12064000  0x2   
himm  0x1206410c  0x17
himm  0x12064110  0x17
himm  0x12064088  0x5
himm  0x1206408c  0x92821e14
himm  0x12064090  0x92821e14
himm  0x12064068  0x51
himm  0x1206406c  0x51
himm  0x120640ac  0x80
himm  0x120640ec  0x11
himm  0x120640f0  0x1111  
himm  0x120640f4  0x33     
himm  0x120641f0  0x1 
himm  0x120641f4  0x0
himm  0x120641f8  0x00800002


echo "++++++++++++++++++++++++++++++++++++++++++++++"
b_cap_online=1

if [ $# -ge 1 ]; then
    b_cap_online=$1
fi

vi_vpss_online_config;
