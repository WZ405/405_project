# $(ROOT)/mpp/code/component/pci/Makefile

ifeq ($(PARAM_FILE), ) 
    PARAM_FILE:=../../Makefile.param
    include $(PARAM_FILE)
endif

VGS_HEADER  := hi_comm_vgs.h
RGN_HEADER  := hi_comm_region.h
PCIV_HEADER := hi_comm_pciv.h
BASE_MKP_HEADER := mkp_ioctl.h dev_ext.h mod_ext.h vb_ext.h mm_ext.h proc_ext.h
SYS_MKP_HEADER  := mkp_sys.h
VGS_EXT_HEADER := vgs_ext.h
SYS_EXT_HEADER := sys_ext.h
RGN_EXT_HEADER := region_ext.h
VPSS_EXT_HEADER:= vpss_ext.h 
HIOS_MCC_USRDEV_HEADER := hi_mcc_usrdev.h

OSAL_HEADER := hi_osal.h osal_list.h osal_ioctl.h osal_mmz.h

INC_DIR=$(MPP_PATH)/component/pci/include

ifeq ($(HIARCH),hi3559av100es)
        VPSS_PATH    := $(MPP_PATH)/cbb/vpss/vpss_hi3559av100es
else
        VPSS_PATH    := $(MPP_PATH)/cbb/vpss/vpss
endif

OSDRV_PATH := ../$(MPP_PATH)

.PHONY: default clean

default:
ifeq ($(MPP_BUILD), y)
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null; rm -f $(BASE_MKP_HEADER); rm -f $(SYS_MKP_HEADER);  popd > /dev/null
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null; rm -f $(VGS_EXT_HEADER); rm -f $(SYS_EXT_HEADER); rm -f $(RGN_EXT_HEADER); rm -f $(VPSS_EXT_HEADER); popd > /dev/null
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null; rm -f $(RGN_HEADER); popd > /dev/null
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null; rm -f $(VGS_HEADER); popd > /dev/null
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null; rm -f $(OSAL_HEADER); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/base/ext_inc > /dev/null;                 cp $(BASE_MKP_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/sys/mkp/include > /dev/null;              cp $(SYS_MKP_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/vgs/ext_inc > /dev/null;                  cp $(VGS_EXT_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/sys/ext_inc > /dev/null;                  cp $(SYS_EXT_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/region/ext_inc > /dev/null;               cp $(RGN_EXT_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(VPSS_PATH)/ext_inc > /dev/null;                         cp $(VPSS_EXT_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/vgs/include > /dev/null;                  cp $(VGS_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(MPP_PATH)/cbb/region/include > /dev/null;               cp $(RGN_HEADER) $(INC_DIR); popd > /dev/null
	@-pushd $(SDK_PATH)/osal/include > /dev/null;                     cp $(OSAL_HEADER) $(INC_DIR); popd > /dev/null

endif

ifeq ($(MPP_BUILD), y)
	@-pushd $(HIWORK_PATH)/osdrv$(HIARCH)$(CVER)/components/pcie_mcc/pcie_mcc/mcc_core/include > /dev/null;   cp $(HIOS_MCC_USRDEV_HEADER) $(REL_INC); popd > /dev/null
else
	@-pushd $(SDK_PATH)/osdrv/components/pcie_mcc/pcie_mcc/mcc_core/include > /dev/null;   cp $(HIOS_MCC_USRDEV_HEADER) $(REL_INC); popd > /dev/null
endif
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null;            cp $(PCIV_HEADER) $(REL_INC); popd > /dev/null
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	   do cd `dirname $$x`; if [ $$? ]; then make || exit 1; cd ../; fi; done

clean:
	@for x in `find ./ -maxdepth 2 -mindepth 2 -name "Makefile" `; \
	   do cd `dirname $$x`; if [ $$? ]; then make clean || exit 1; cd ../; fi; done

ifeq ($(MPP_BUILD), y)
	@-pushd $(MPP_PATH)/component/pci/include > /dev/null; popd > /dev/null
endif

