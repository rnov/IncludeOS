#################################################
#          IncludeOS SERVICE makefile           #
#################################################

# IncludeOS location
ifndef INCLUDEOS_INSTALL
	INCLUDEOS_INSTALL=$(HOME)/IncludeOS_install
endif

ifneq ($(DISK),)
MEMDISK=memdisk.o
endif

.SUFFIXES: .asm

# Maximum memory in megabytes
# Used if memory info is not otherwise available such as via multiboot
ifeq ($(MAX_MEM),)
MAX_MEM=128
endif

# Shorter name
INSTALL = $(INCLUDEOS_INSTALL)

# Compiler and linker options
###################################################
CAPABS_COMMON = -mstackrealign -msse3 -fstack-protector-strong -DOS_TERMINATE_ON_CONTRACT_VIOLATION $(LOCAL_DEFS)
WARNS  = -Wall -Wextra #-pedantic
DEBUG_OPTS = -ggdb3 -v

OPTIONS = $(WARNS) $(CAPABS) $(EXTRA_FLAGS)

# External Libraries
###################################################
LIBC_OBJ = $(INSTALL)/newlib/libc.a
LIBG_OBJ = $(INSTALL)/newlib/libg.a
LIBM_OBJ = $(INSTALL)/newlib/libm.a

LIBGCC = $(INSTALL)/libgcc/libgcc.a
LIBCXX = $(INSTALL)/libcxx/libc++.a $(INSTALL)/libcxx/libc++abi.a


INC_NEWLIB=$(INSTALL)/newlib/include
INC_LIBCXX=$(INSTALL)/libcxx/include

CC  = $(shell command -v clang-3.8 || command -v clang-3.6) -target i686-elf
CPP = $(shell command -v clang++-3.8 || command -v clang++-3.6 || command -v clang++) -target i686-elf
ifndef LD_INC
	LD_INC = ld
endif
ifndef OBJCOPY_INC
	OBJCOPY_INC=objcopy
endif

INCLUDES = $(LOCAL_INCLUDES) -I$(INC_LIBCXX) -I$(INSTALL)/api/sys -I$(INC_NEWLIB) -I$(INSTALL)/api -I$(INSTALL)/mod/GSL

all:      CAPABS = $(CAPABS_COMMON) -O2
debug:    CAPABS = $(CAPABS_COMMON) -O0
stripped: CAPABS = $(CAPABS_COMMON) -O2

# by default, don't use strip because it has caused us some problems
STRIPPED=--strip-debug

CPPOPTS = $(CAPABS) $(WARNS) -c -m32 -std=c++14 $(INCLUDES) -D_LIBCPP_HAS_NO_THREADS=1 -D_GNU_SOURCE
LDOPTS = -nostdlib -melf_i386 -N --eh-frame-hdr --script=$(INSTALL)/linker.ld --defsym _MAX_MEM_MIB_=$(MAX_MEM) --defsym=__stack_rand_ba=`date +%s`

# Objects
###################################################

CRTBEGIN_OBJ = $(INSTALL)/crt/crtbegin.o
CRTEND_OBJ = $(INSTALL)/crt/crtend.o
CRTI_OBJ = $(INSTALL)/crt/crti.o
CRTN_OBJ = $(INSTALL)/crt/crtn.o
MULTIBOOT = $(INSTALL)/multiboot.o

DRIVER_OBJS_EXT = $(DRIVERS:=.o)
DRIVER_OBJS = $(addprefix $(INSTALL)/drivers/, $(DRIVER_OBJS_EXT))

PLATFORM_OBJS_EXT = $(PLATFORM:=.o)
PLATFORM_OBJS = $(addprefix $(INSTALL)/platforms/, $(PLATFORM_OBJS_EXT))


# Full link list
OBJS  =  $(FILES:.cpp=.o)
OBJS  += .service_name.o

LIBS =  $(INSTALL)/os.a $(LIBCXX) $(INSTALL)/os.a $(LIBC_OBJ) $(LIBM_OBJ) $(LIBGCC)

OS_PRE = $(CRTBEGIN_OBJ) $(CRTI_OBJ) $(MULTIBOOT)
OS_POST = $(CRTEND_OBJ) $(CRTN_OBJ)

DEPS = $(OBJS:.o=.d)

# Complete build
###################################################
# A complete build includes:
# - a "service", to be linked with OS-objects (OS included)
.PHONY: all stripped debug debug-info debug-all memdisk service

all: LDOPTS += --strip-debug
all: STRIPPED=--strip-all
all: service

stripped: LDOPTS  += -s
stripped: STRIPPED=--strip-all
stripped: service

# Build like "all" but with debugging output (i.e. the 'debug'-macro) enabled
debug-info: CAPABS += -UNO_DEBUG
debug-info: service

# Build with debugging symbols (OBS: Dramatically increases binary size)
debug: CCOPTS  += $(DEBUG_OPTS)
debug: CPPOPTS += $(DEBUG_OPTS)
debug: OBJ_LIST += $(LIBG_OBJ)
debug: CPPOPTS += -O0
debug: STRIPPED=
debug: service

# Build with debugging symbols + debugging ouput, i.e. "debug" + "debug-info"
debug-all: CAPABS += -UNO_DEBUG
debug-all: CCOPTS  += $(DEBUG_OPTS)
debug-all: CPPOPTS += $(DEBUG_OPTS)
debug-all: OBJ_LIST += $(LIBG_OBJ)
debug-all: CPPOPTS += -O0
debug-all: service

minimal: stripped
minimal: CPPOPTS += -Os

# Service
###################################################

# Link the service with the os
service: $(OBJS) $(LIBS) $(DRIVER_OBJS) $(PLATFORM_OBJS)
# build memdisk only when MEMDISK is set
ifneq ($(MEMDISK),)
	@echo "\n>> Creating memdisk"
	python $(INSTALL)/memdisk/memdisk.py --file $(INSTALL)/memdisk/memdisk.asm $(DISK)
	@echo "\n>> Assembling memdisk"
	nasm -f elf $(INSTALL)/memdisk/memdisk.asm -o $(MEMDISK)
endif
	@echo "\n>> Linking service with OS"
	$(LD_INC) $(LDOPTS) $(OS_PRE) $(OBJS) $(DRIVER_OBJS) $(PLATFORM_OBJS) $(EXTRA_LIBS) $(LIBS) $(OS_POST) $(MEMDISK) -o $(SERVICE)
	@echo "\n>> Adding symbol section"
	$(INSTALL)/elf_syms $(SERVICE)
	$(OBJCOPY_INC) --update-section .elf_symbols=_elf_symbols.bin $(SERVICE) $(SERVICE)
	$(RM) _elf_symbols.bin
# stripping must be done after symbol section work
ifneq ($(STRIPPED),)
	strip $(STRIPPED) --remove-section=.comment $(SERVICE)
endif
	@echo "\n>> Building" $(SERVICE).img
	$(INSTALL)/vmbuild $(SERVICE) $(INSTALL)/bootloader

.service_name.o: $(INSTALL)/service_name.cpp
	$(CPP) $(CPPOPTS) -DSERVICE_NAME="\"$(SERVICE_NAME)\"" -o $@ $<

servicefile:
	@echo $(SERVICE).img

# Object files
###################################################

# Runtime
crt%.o: $(INSTALL)/crt/crt%.s
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

# General C++-files to object files
%.o: %.cpp
	$(CPP) -MMD $(CPPOPTS) -o $@ $<

# nasm-assembled object files
%.o: %.asm
	nasm -f elf -o $@ $<

%.o: %.c
	$(CC) -MMD $(WARNS) $(CAPABS) $(EXTRA_FLAGS) -c -m32 -std=gnu11 -I$(INSTALL)/api/sys -I$(INC_NEWLIB) -o $@ $<

# AS-assembled object files
%.o: %.s
	$(CPP) $(CPPOPTS) -x assembler-with-cpp $<

# Cleanup
###################################################
clean:
	$(RM) $(OBJS) $(DEPS)
	$(RM) $(SERVICE) $(SERVICE).img $(MEMDISK)

-include $(DEPS)
