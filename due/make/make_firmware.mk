include $(shell pwd)/../make/sources

print-%  : ; @echo $* = $($*)

BUILD_DIR := release

ROOT := $(realpath $(shell dirname '$(dir $(lastword $(MAKEFILE_LIST)))'))
CROSS_COMPILE = $(ROOT)/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-
AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AS = $(CROSS_COMPILE)as
NM = $(CROSS_COMPILE)nm
LKELF = $(CROSS_COMPILE)g++
OBJCP = $(CROSS_COMPILE)objcopy
SIZE  = $(CROSS_COMPILE)size
RM=rm -Rf
MKDIR=mkdir -p

OPTIMIZATION = -g -O0 -DDEBUG
#-------------------------------------------------------------------------------
#  Flags
#-------------------------------------------------------------------------------
CFLAGS += -Wall --param max-inline-insns-single=500 -mcpu=cortex-m3 -mthumb -mlong-calls
CFLAGS += -ffunction-sections -fdata-sections -nostdlib -std=c99
CFLAGS += $(OPTIMIZATION) $(INCLUDES)
#CFLAGS += -Dprintf=iprintf
CPPFLAGS += -Wall --param max-inline-insns-single=500 -mcpu=cortex-m3 -mthumb -mlong-calls -nostdlib
CPPFLAGS += -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -std=c++98
CPPFLAGS += $(OPTIMIZATION) $(INCLUDES)
#CPPFLAGS += -Dprintf=iprintf
ASFLAGS = -mcpu=cortex-m3 -mthumb -Wall -g $(OPTIMIZATION) $(INCLUDES)
ARFLAGS = rcs

LNK_SCRIPT=$(ROOT)/sam/linker_scripts/gcc/flash.ld
LIBSAM_ARCHIVE=$(ROOT)/lib/libsam_sam3x8e_gcc_rel.a

UPLOAD_PORT_BASENAME=$(patsubst /dev/%,%,$(UPLOAD_PORT))

#-------------------------------------------------------------------------------
# High verbosity flags
#-------------------------------------------------------------------------------
ifdef VERBOSE
CFLAGS += -Wall -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int
CFLAGS += -Werror-implicit-function-declaration -Wmain -Wparentheses
CFLAGS += -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused
CFLAGS += -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef
CFLAGS += -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings
CFLAGS += -Wsign-compare -Waggregate-return -Wstrict-prototypes
CFLAGS += -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations
CFLAGS += -Wredundant-decls -Wnested-externs -Winline -Wlong-long
CFLAGS += -Wunreachable-code
CFLAGS += -Wcast-align
CFLAGS += -Wmissing-noreturn
CFLAGS += -Wconversion
CPPFLAGS += -Wall -Wchar-subscripts -Wcomment -Wformat=2
CPPFLAGS += -Wmain -Wparentheses -Wcast-align -Wunreachable-code
CPPFLAGS += -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused
CPPFLAGS += -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef
CPPFLAGS += -Wshadow -Wpointer-arith -Wwrite-strings
CPPFLAGS += -Wsign-compare -Waggregate-return -Wmissing-declarations
CPPFLAGS += -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations
CPPFLAGS += -Wpacked -Wredundant-decls -Winline -Wlong-long
CPPFLAGS += -Wmissing-noreturn
CPPFLAGS += -Wconversion
UPLOAD_VERBOSE_FLAGS += -i -d
endif

C_SRCS := $(SOURCES:.c=.o)
COMMON_INC = $(foreach d, $(COMMON_INC_PATH), -I $d)
C_OBJS := $(patsubst %,$(BUILD_DIR)/%,$(C_SRCS))
OBJ_DIRS = $(sort $(dir $(C_OBJS)))

CRUFT=$(shell find . -name '*.o' -o -name '*.dump' -o -name '*.hex' -o -name '*.bin' -o -name '*.map' -o -name '*.elf' -o -name '*.a' -o -name '*.a.txt')

$(shell mkdir -p $(OBJ_DIRS))

.PHONY: all clean

all: binary size

size: $(BUILD_DIR)/$(TARGET).elf
	@$(SIZE) $^
	@echo ""
	du -b $(BUILD_DIR)/*.bin
	
binary: $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$Q $(OBJCP) -O ihex $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$Q $(OBJCP) -O binary $< $@

$(BUILD_DIR)/$(TARGET).a: $(C_OBJS)
	"$(AR)" $(ARFLAGS) $@ $^
	"$(NM)" $@ > $@.txt

#  -> .elf
$(BUILD_DIR)/$(TARGET).elf: $(BUILD_DIR)/$(TARGET).a
	"$(LKELF)" -Os -Wl,--gc-sections -mcpu=cortex-m3 \
	  "-T$(LNK_SCRIPT)" "-Wl,-Map,$(BUILD_DIR)/$(TARGET).map" \
	  -o $@ \
	  "-L$(BUILD_DIR)" \
	  -lm -lgcc -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections \
	  -Wl,--entry=Reset_Handler -Wl,--unresolved-symbols=report-all -Wl,--warn-common \
	  -Wl,--warn-section-align -Wl,--warn-unresolved-symbols \
	  -Wl,--start-group \
	  $^ $(LIBSAM_ARCHIVE) \
	  -Wl,--end-group

$(C_OBJS): $(BUILD_DIR)/%.o: %.c
	$Q $(CC) $(COMMON_INC) $(CFLAGS) -c $< -o $@
	
	
clean:
	rm -fr $(CRUFT) $(BUILD_DIR)
	
