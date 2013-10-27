#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

ifeq ($(PLATFORM),cube)
include $(DEVKITPPC)/gamecube_rules
else
export PLATFORM := wii
include $(DEVKITPPC)/wii_rules
endif

# Filter output
PIPE_TO_SED := 2>&1 | sed "s/:\([0-9]*\):/\(\1\) :/"

TARGET		:= lib-$(PLATFORM)/libphysfs.a
BUILD		:= build-$(PLATFORM)
SOURCES		:= physfs.c physfs_byteorder.c physfs_unicode.c \
			platform/wii.c archivers/dir.c archivers/grp.c \
			archivers/hog.c archivers/lzma.c archivers/mvl.c \
			archivers/qpak.c archivers/wad.c archivers/zip.c

INCLUDE		:= -I$(CURDIR) -I$(CURDIR)/$(BUILD) -I$(LIBOGC_INC) \
			-I$(PORTLIBS)/include

CFLAGS		:= -g -O2 -Wall $(MACHDEP) $(INCLUDE) -DPHYSFS_SUPPORTS_ZIP \
			-DPHYSFS_SUPPORTS_7Z_NOT -DPHYSFS_NO_CDROM_SUPPORT \
			-DPHYSFS_NO_THREAD_SUPPORT

OUTPUT		:= $(CURDIR)/$(TARGET)

OFILES		:= $(addprefix $(BUILD)/,$(SOURCES:.c=.o))

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
all: $(OUTPUT)
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr build-wii build-cube lib

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT): $(OFILES)
	mkdir -p lib-$(PLATFORM)
	@rm -f "$(OUTPUT)"
	@$(AR) rcs "$(OUTPUT)" $(OFILES)
	@echo built ... $(notdir $@)

$(BUILD)/%.o: %.c
	@echo Compiling $<
	@-mkdir -p $(dir $@)
	@powerpc-eabi-gcc $(CFLAGS) -c $< -o $@ $(PIPE_TO_SED)

