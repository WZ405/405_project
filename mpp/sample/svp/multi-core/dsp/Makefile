# Hisilicon Hi35xx sample Makefile
include ../Makefile.param

CFLAGS += -I$(PWD)/sample
CFLAGS += -I$(PWD)/encapsulation/include

SRCS := $(wildcard *.c)
SRCS += $(wildcard ./sample/*.c)
SRCS += $(wildcard ./encapsulation/src/*.c)
SRCS += $(wildcard $(PWD)/../common/*.c)
TARGET := sample_dsp_main
CFLAGS += -L$(PWD)/libs
CFLAGS += -lopencv_highgui -lopencv_core -lopencv_imgproc -lopencv_imgcodecs

# compile linux or HuaweiLite
include $(PWD)/../../../$(OSTYPE).mak
