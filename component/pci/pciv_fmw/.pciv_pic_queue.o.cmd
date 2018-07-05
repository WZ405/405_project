cmd_/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o := aarch64-himix100-linux-gcc -Wp,-MD,/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/.pciv_pic_queue.o.d  -nostdinc -isystem /opt/hisi-linux/x86-arm/aarch64-himix100-linux/host_bin/../lib/gcc/aarch64-linux-gnu/6.2.1/include -I./arch/arm64/include -I./arch/arm64/include/generated/uapi -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 -fno-PIE -mgeneral-regs-only -DCONFIG_AS_LSE=1 -fno-asynchronous-unwind-tables -mpc-relative-literal-loads -fno-delete-null-pointer-checks -Wno-frame-address -O2 --param=allow-store-data-races=0 -DCC_HAVE_ASM_GOTO -Wframe-larger-than=2048 -fno-stack-protector -Wno-unused-but-set-variable -Wno-unused-const-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -Werror=implicit-int -Werror=strict-prototypes -Werror=date-time -Werror=incompatible-pointer-types -I/home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/include -I/home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/pciv/kernel -I/home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include -I. -Wall -Wno-error=implicit-function-declaration -DVER_X=2 -DVER_Y=0 -DVER_Z=0 -DVER_P=6 -DVER_B=20 -DUSER_BIT_64 -DKERNEL_BIT_64 -Wno-date-time -Wall -Dhi3559av100 -DHI_XXXX  -DMODULE -mcmodel=large  -DKBUILD_BASENAME='"pciv_pic_queue"'  -DKBUILD_MODNAME='"hi3559av100_pciv_fmw"' -c -o /home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o /home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.c

source_/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o := /home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.c

deps_/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o := \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/include/mm_ext.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/include/hi_osal.h \
    $(wildcard include/config/compat.h) \
    $(wildcard include/config/hisi/snapshot/boot.h) \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/include/osal_list.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/include/osal_ioctl.h \
  /opt/hisi-linux/x86-arm/aarch64-himix100-linux/lib/gcc/aarch64-linux-gnu/6.2.1/include/stdarg.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_math.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_type.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_common.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_defines.h \
    $(wildcard include/config/hi/chip/type.h) \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/autoconf.h \
    $(wildcard include/config/hi3559av100.h) \
    $(wildcard include/config/hi/arch.h) \
    $(wildcard include/config/cpu/type/multi/core.h) \
    $(wildcard include/config/cpu/type.h) \
    $(wildcard include/config/version/asic.h) \
    $(wildcard include/config/hi/fpga.h) \
    $(wildcard include/config/linux/os.h) \
    $(wildcard include/config/os/type.h) \
    $(wildcard include/config/linux/4/9/y.h) \
    $(wildcard include/config/kernel/version.h) \
    $(wildcard include/config/kernel/aarch64/himix100.h) \
    $(wildcard include/config/hi/cross.h) \
    $(wildcard include/config/libc/type.h) \
    $(wildcard include/config/kernel/bit.h) \
    $(wildcard include/config/user/aarch64/himix100.h) \
    $(wildcard include/config/hi/cross/lib.h) \
    $(wildcard include/config/user/bit.h) \
    $(wildcard include/config/release/type/release.h) \
    $(wildcard include/config/hi/rls/mode.h) \
    $(wildcard include/config/hi/vi/support.h) \
    $(wildcard include/config/hi/vi/mipi.h) \
    $(wildcard include/config/hi/isp/support.h) \
    $(wildcard include/config/hi/dis/support.h) \
    $(wildcard include/config/hi/vpss/support.h) \
    $(wildcard include/config/hi/avs/support.h) \
    $(wildcard include/config/hi/gdc/support.h) \
    $(wildcard include/config/hi/vgs/support.h) \
    $(wildcard include/config/hi/chnl/support.h) \
    $(wildcard include/config/hi/venc/support.h) \
    $(wildcard include/config/hi/h265e/support.h) \
    $(wildcard include/config/hi/h264e/support.h) \
    $(wildcard include/config/hi/jpege/support.h) \
    $(wildcard include/config/hi/prores/support.h) \
    $(wildcard include/config/hi/lowdelay/support.h) \
    $(wildcard include/config/hi/jpege/dcf.h) \
    $(wildcard include/config/hi/vdec/support.h) \
    $(wildcard include/config/hi/h265d/support.h) \
    $(wildcard include/config/hi/h264d/support.h) \
    $(wildcard include/config/vdec/ip.h) \
    $(wildcard include/config/hi/jpegd/support.h) \
    $(wildcard include/config/hi/vo/support.h) \
    $(wildcard include/config/hi/vo/hd.h) \
    $(wildcard include/config/hi/vo/play/ctl.h) \
    $(wildcard include/config/hi/vo/luma.h) \
    $(wildcard include/config/hi/vo/cover/osd.h) \
    $(wildcard include/config/hi/vo/vgs.h) \
    $(wildcard include/config/hi/vo/graph.h) \
    $(wildcard include/config/hi/region/support.h) \
    $(wildcard include/config/hi/audio/support.h) \
    $(wildcard include/config/hi/acodec/support.h) \
    $(wildcard include/config/hi/acodec/version.h) \
    $(wildcard include/config/hi/acodec/max/gain.h) \
    $(wildcard include/config/hi/acodec/min/gain.h) \
    $(wildcard include/config/hi/acodec/gain/step.h) \
    $(wildcard include/config/hi/acodec/fast/power/support.h) \
    $(wildcard include/config/hi/recordvqe/support.h) \
    $(wildcard include/config/hi/hdr/support.h) \
    $(wildcard include/config/drv.h) \
    $(wildcard include/config/extdrv.h) \
    $(wildcard include/config/interdrv.h) \
    $(wildcard include/config/cipher.h) \
    $(wildcard include/config/hiuser.h) \
    $(wildcard include/config/mipi/tx.h) \
    $(wildcard include/config/mipi/rx.h) \
    $(wildcard include/config/hi/ir.h) \
    $(wildcard include/config/hi/wdg.h) \
    $(wildcard include/config/hi/hdmi/support.h) \
    $(wildcard include/config/hi/hifb/support.h) \
    $(wildcard include/config/hi/svp/support.h) \
    $(wildcard include/config/hi/svp/dsp.h) \
    $(wildcard include/config/hi/svp/liteos.h) \
    $(wildcard include/config/hi/svp/cnn.h) \
    $(wildcard include/config/hi/svp/ive.h) \
    $(wildcard include/config/hi/svp/md.h) \
    $(wildcard include/config/hi/svp/dpu/rect.h) \
    $(wildcard include/config/hi/svp/dpu/match.h) \
    $(wildcard include/config/hi/photo/support.h) \
    $(wildcard include/config/hi/tde/support.h) \
    $(wildcard include/config/hi/pciv/support.h) \
    $(wildcard include/config/hi/avs/lut/support.h) \
    $(wildcard include/config/hi/gdb/no.h) \
    $(wildcard include/config/hi/gdb.h) \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/component/pci/include/osal_mmz.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_debug.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_common.h \
  /home/pub/himpp_git/himpp/board/mpp/./../mpp/out/hi3559av100/linux/multi-core/include/hi_comm_video.h \
    $(wildcard include/config/info/s.h) \
  /home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.h \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  arch/arm64/include/generated/asm/types.h \
  include/uapi/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm64/include/uapi/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  include/uapi/linux/posix_types.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/kasan.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
    $(wildcard include/config/gcov/kernel.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  arch/arm64/include/uapi/asm/posix_types.h \
  include/uapi/asm-generic/posix_types.h \
  include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
    $(wildcard include/config/page/poisoning/zero.h) \
  include/uapi/linux/const.h \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/panic/timeout.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  include/linux/linkage.h \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
    $(wildcard include/config/unused/symbols.h) \
  arch/arm64/include/asm/linkage.h \
  include/linux/bitops.h \
  arch/arm64/include/asm/bitops.h \
  arch/arm64/include/asm/barrier.h \
  include/asm-generic/barrier.h \
    $(wildcard include/config/smp.h) \
  include/asm-generic/bitops/builtin-__ffs.h \
  include/asm-generic/bitops/builtin-ffs.h \
  include/asm-generic/bitops/builtin-__fls.h \
  include/asm-generic/bitops/builtin-fls.h \
  include/asm-generic/bitops/ffz.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/lock.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/le.h \
  arch/arm64/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  arch/arm64/include/generated/asm/swab.h \
  include/uapi/asm-generic/swab.h \
  include/linux/byteorder/generic.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/typecheck.h \
  include/linux/printk.h \
    $(wildcard include/config/message/loglevel/default.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk/nmi.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/debug/rodata.h) \
  include/linux/kern_levels.h \
  include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/uapi/linux/kernel.h \
  include/uapi/linux/sysinfo.h \
  arch/arm64/include/asm/cache.h \
  arch/arm64/include/asm/cachetype.h \
  arch/arm64/include/asm/cputype.h \
  arch/arm64/include/asm/sysreg.h \
    $(wildcard include/config/arm64/4k/pages.h) \
    $(wildcard include/config/arm64/16k/pages.h) \
    $(wildcard include/config/arm64/64k/pages.h) \
  arch/arm64/include/asm/opcodes.h \
    $(wildcard include/config/cpu/big/endian.h) \
    $(wildcard include/config/cpu/endian/be8.h) \
  arch/arm64/include/../../arm/include/asm/opcodes.h \
    $(wildcard include/config/cpu/endian/be32.h) \
    $(wildcard include/config/thumb2/kernel.h) \

/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o: $(deps_/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o)

$(deps_/home/pub/himpp_git/himpp/board/mpp/component/pci/pciv_fmw/pciv_pic_queue.o):
