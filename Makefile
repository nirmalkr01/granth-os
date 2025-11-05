ARCH := x86_64
TARGET := kernel.elf
OUT := out
SRC := src
CC := clang
LD := ld.lld
AS := nasm

CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -O2 -Wall -Wextra -std=c17 -Isrc/include -Isrc
LDFLAGS := -nostdlib \
  -z common-page-size=0x1000 \
  -z max-page-size=0x1000 \
  -z separate-loadable-segments \
  -z separate-code \
  -T linker.ld

EFI_DIR := $(OUT)/iso/EFI/BOOT
BOOT_DIR := $(OUT)/iso/boot
ISO := $(OUT)/GranthOS.iso
FW_DIR := $(OUT)/fw
FW_CODE := $(FW_DIR)/OVMF_CODE.fd
FW_VARS := $(FW_DIR)/OVMF_VARS.fd

ifeq ($(wildcard /mnt/c/limine-10.2.1-binary/BOOTX64.EFI),)
  LIMINE_DIR_WIN := /mnt/c/dev/limine
else
  LIMINE_DIR_WIN := /mnt/c/limine-10.2.1-binary
endif
LIMINE_BOOT_EFI := $(LIMINE_DIR_WIN)/BOOTX64.EFI
LIMINE_UEFI_CD  := $(LIMINE_DIR_WIN)/limine-uefi-cd.bin

OVMF_CODE_CANDIDATES := \
  /usr/share/OVMF/OVMF_CODE_4M.fd \
  /usr/share/OVMF/OVMF_CODE.fd \
  /usr/share/ovmf/OVMF_CODE.fd \
  /usr/share/qemu/OVMF_CODE.fd \
  /usr/share/edk2/ovmf/OVMF_CODE.fd

OVMF_VARS_CANDIDATES := \
  /usr/share/OVMF/OVMF_VARS_4M.fd \
  /usr/share/OVMF/OVMF_VARS.fd \
  /usr/share/ovmf/OVMF_VARS.fd \
  /usr/share/qemu/OVMF_VARS.fd \
  /usr/share/edk2/ovmf/OVMF_VARS.fd

OVMF_CODE_SYS := $(shell for p in $(OVMF_CODE_CANDIDATES); do [ -f $$p ] && echo $$p && break; done)
OVMF_VARS_SYS := $(shell for p in $(OVMF_VARS_CANDIDATES); do [ -f $$p ] && echo $$p && break; done)

C_SRCS  := $(shell find $(SRC) -name '*.c')
ASM_SRCS:= $(shell find $(SRC) -name '*.s' -o -name '*.S' -o -name '*.asm')
OBJS    := $(patsubst $(SRC)/%, $(OUT)/%, $(C_SRCS:.c=.o)) \
           $(patsubst $(SRC)/%, $(OUT)/%, $(ASM_SRCS:.s=.o))

all: $(OUT)/$(TARGET) iso

$(OUT)/$(TARGET): $(OBJS) linker.ld
	@mkdir -p $(OUT)
	$(LD) $(LDFLAGS) $(OBJS) -o $(OUT)/$(TARGET)

$(OUT)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/%.o: $(SRC)/%.s
	@mkdir -p $(dir $@)
	$(AS) -felf64 $< -o $@

$(OUT)/%.o: $(SRC)/%.S
	@mkdir -p $(dir $@)
	$(CC) -x assembler-with-cpp -c $< -o $@

$(OUT)/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(AS) -felf64 $< -o $@

iso: $(OUT)/$(TARGET) limine_files
	@mkdir -p $(EFI_DIR) $(BOOT_DIR)
	cp $(LIMINE_BOOT_EFI) $(EFI_DIR)/BOOTX64.EFI
	cp $(OUT)/$(TARGET) $(BOOT_DIR)/kernel.elf
	cp limine.cfg $(OUT)/iso/limine.cfg
	cp limine.conf $(OUT)/iso/limine.conf || true
	cp $(LIMINE_UEFI_CD) $(BOOT_DIR)/limine-uefi-cd.bin
	xorriso -as mkisofs \
	  --efi-boot boot/limine-uefi-cd.bin \
	  -efi-boot-part --efi-boot-image --protective-msdos-label \
	  -R -V "GranthOS" \
	  -o $(ISO) $(OUT)/iso

firmware: $(FW_CODE) $(FW_VARS)
	@true

$(FW_CODE):
	@mkdir -p $(FW_DIR)
	cp "$(OVMF_CODE_SYS)" "$(FW_CODE)"

$(FW_VARS):
	@mkdir -p $(FW_DIR)
	cp "$(OVMF_VARS_SYS)" "$(FW_VARS)"

run-qemu: iso firmware
	@echo "Using firmware:"; echo "  CODE: $(FW_CODE)"; echo "  VARS: $(FW_VARS)"
	qemu-system-x86_64 -machine q35 -m 512M \
	  -drive if=pflash,format=raw,readonly=on,file="$(FW_CODE)" \
	  -drive if=pflash,format=raw,file="$(FW_VARS)" \
	  -drive file=$(ISO),media=cdrom,if=ide \
	  -no-reboot -no-shutdown \
	  -display default -serial stdio

run-seabios: iso
	qemu-system-x86_64 -machine q35 -m 512M \
	  -drive file=$(ISO),media=cdrom,if=ide \
	  -no-reboot -no-shutdown \
	  -display default -serial stdio

limine_files:
	test -f $(LIMINE_BOOT_EFI)
	test -f $(LIMINE_UEFI_CD)

clean:
	rm -rf $(OUT)

.PHONY: all iso run-qemu run-seabios clean limine_files firmware
