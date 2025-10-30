ARCH := x86_64
TARGET := kernel.elf
OUT := out
SRC := src
CC := clang
LD := ld.lld
AS := nasm

CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -O2 -Wall -Wextra -std=c17
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T linker.ld

EFI_DIR := $(OUT)/iso/EFI/BOOT
BOOT_DIR := $(OUT)/iso/boot
ISO := $(OUT)/GranthOS.iso
FW_DIR := $(OUT)/fw
FW_CODE := $(FW_DIR)/OVMF_CODE.fd
FW_VARS := $(FW_DIR)/OVMF_VARS.fd

# --- Limine path autodetect (prefers C:\limine-10.2.1-binary) ---
ifeq ($(wildcard /mnt/c/limine-10.2.1-binary/BOOTX64.EFI),)
  LIMINE_DIR_WIN := /mnt/c/dev/limine
else
  LIMINE_DIR_WIN := /mnt/c/limine-10.2.1-binary
endif
LIMINE_BOOT_EFI := $(LIMINE_DIR_WIN)/BOOTX64.EFI
LIMINE_UEFI_CD  := $(LIMINE_DIR_WIN)/limine-uefi-cd.bin

# --- Robust OVMF detection (CODE + VARS), prefer 4M images ---
OVMF_CODE_CANDIDATES := \
  /usr/share/OVMF/OVMF_CODE_4M.fd \
  /usr/share/OVMF/OVMF_CODE.fd \
  /usr/share/ovmf/OVMF_CODE.fd \
  /usr/share/qemu/OVMF_CODE.fd \
  /usr/share/edk2/ovmf/OVMF_CODE.fd \
  /usr/share/OVMF/OVMF_CODE_4M.secboot.fd \
  /usr/share/OVMF/OVMF_CODE_4M.ms.fd \
  /usr/share/OVMF/OVMF_CODE_4M.snakeoil.fd

OVMF_VARS_CANDIDATES := \
  /usr/share/OVMF/OVMF_VARS_4M.fd \
  /usr/share/OVMF/OVMF_VARS.fd \
  /usr/share/ovmf/OVMF_VARS.fd \
  /usr/share/qemu/OVMF_VARS.fd \
  /usr/share/edk2/ovmf/OVMF_VARS.fd \
  /usr/share/OVMF/OVMF_VARS_4M.ms.fd \
  /usr/share/OVMF/OVMF_VARS_4M.snakeoil.fd

OVMF_CODE_SYS := $(shell for p in $(OVMF_CODE_CANDIDATES); do [ -f $$p ] && echo $$p && break; done)
OVMF_VARS_SYS := $(shell for p in $(OVMF_VARS_CANDIDATES); do [ -f $$p ] && echo $$p && break; done)

ifeq ($(OVMF_CODE_SYS),)
  $(warning Could not find OVMF_CODE*.fd. Try: sudo apt install ovmf)
endif
ifeq ($(OVMF_VARS_SYS),)
  $(warning Could not find OVMF_VARS*.fd. Try: sudo apt install ovmf)
endif

all: $(OUT)/$(TARGET) iso

$(OUT)/$(TARGET): $(SRC)/boot.s $(SRC)/kernel.c linker.ld
	@mkdir -p $(OUT)
	$(AS) -felf64 $(SRC)/boot.s -o $(OUT)/boot.o
	$(CC) $(CFLAGS) -c $(SRC)/kernel.c -o $(OUT)/kernel.o
	$(LD) $(LDFLAGS) $(OUT)/boot.o $(OUT)/kernel.o -o $(OUT)/$(TARGET)

iso: $(OUT)/$(TARGET) limine_files
	@mkdir -p $(EFI_DIR) $(BOOT_DIR)
	cp $(LIMINE_BOOT_EFI) $(EFI_DIR)/BOOTX64.EFI
	cp $(OUT)/$(TARGET) $(BOOT_DIR)/kernel.elf
	cp limine.conf $(OUT)/iso/limine.conf
	cp $(LIMINE_UEFI_CD) $(BOOT_DIR)/limine-uefi-cd.bin
	xorriso -as mkisofs \
	  --efi-boot boot/limine-uefi-cd.bin \
	  -efi-boot-part --efi-boot-image --protective-msdos-label \
	  -R -V "GranthOS" \
	  -o $(ISO) $(OUT)/iso

# Prepare local copies of firmware to avoid path/permission issues
firmware: $(FW_CODE) $(FW_VARS)
	@true

$(FW_CODE):
	@mkdir -p $(FW_DIR)
	cp "$(OVMF_CODE_SYS)" "$(FW_CODE)"

$(FW_VARS):
	@mkdir -p $(FW_DIR)
	cp "$(OVMF_VARS_SYS)" "$(FW_VARS)"

# Run with pflash (reliable across distros)
run-qemu: iso firmware
	@echo "Using firmware:"
	@echo "  CODE: $(FW_CODE)"
	@echo "  VARS: $(FW_VARS)"
	qemu-system-x86_64 -machine q35 -m 512M \
	  -drive if=pflash,format=raw,readonly=on,file="$(FW_CODE)" \
	  -drive if=pflash,format=raw,file="$(FW_VARS)" \
	  -drive file=$(ISO),media=cdrom,if=ide \
	  -display default -serial stdio

# Direct legacy (SeaBIOS) test if needed (won't boot UEFI images)
run-seabios: iso
	qemu-system-x86_64 -machine q35 -m 512M \
	  -drive file=$(ISO),media=cdrom,if=ide \
	  -display default -serial stdio

limine_files:
	test -f $(LIMINE_BOOT_EFI)
	test -f $(LIMINE_UEFI_CD)

clean:
	rm -rf $(OUT)

.PHONY: all iso run-qemu run-seabios clean limine_files firmware
