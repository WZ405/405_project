
obj-m += $(HIARCH)_pciv.o
$(HIARCH)_pciv-y := pciv.o pciv_ext.o pciv_drvadp.o

EXTRA_CFLAGS += $(FLAGS)
ifeq ($(HIGDB),HI_GDB)
EXTRA_CFLAGS += -g
endif

EXTRA_CFLAGS += $(MPP_CFLAGS)
#EXTRA_CFLAGS += -Wall -D$(HIARCH) -D$(HIDBG) -D$(HI_FPGA)
EXTRA_CFLAGS += -Wall -D$(HIARCH) -D$(HI_FPGA)

all:
	@echo "EXTRA_CFLAGS = $(EXTRA_CFLAGS)"
	@make -C $(KERNEL_ROOT) M=$(PWD) modules	
	@cp $(HIARCH)_pciv.ko $(REL_KO)/

clean:
	@make -C $(KERNEL_ROOT) M=$(PWD) clean
	@rm -rf $(REL_KO)/$(HIARCH)_pciv.ko
