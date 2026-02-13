# GranthOS

A minimal x86_64 operating system kernel built from scratch using the Limine bootloader protocol.

## Features

- x86_64 architecture support
- UEFI boot via Limine bootloader
- Hardware Abstraction Layer (HAL)
- Memory management:
  - Physical Memory Manager (PMM)
  - Virtual Memory Manager (VMM)
  - Kernel heap allocator
- CPU architecture support:
  - Global Descriptor Table (GDT)
  - Interrupt Descriptor Table (IDT)
  - Task State Segment (TSS)
  - Programmable Interrupt Controller (PIC)
  - Programmable Interval Timer (PIT)
- Framebuffer graphics support
- Serial port logging (COM1)

## Prerequisites

- `clang` - C compiler
- `ld.lld` - LLVM linker
- `nasm` - Assembler
- `xorriso` - ISO creation tool
- `qemu-system-x86_64` - Emulator for testing
- OVMF firmware files (for UEFI emulation)
- Limine bootloader binaries

### Limine Setup

Download Limine bootloader binaries and ensure these files are available:
- `BOOTX64.EFI`
- `limine-uefi-cd.bin`

Update the `LIMINE_DIR_WIN` path in the Makefile to point to your Limine installation.

## Building

Build the kernel and create a bootable ISO:

```bash
make
```

This will:
1. Compile all C and assembly sources
2. Link the kernel ELF binary
3. Create a bootable ISO image at `out/GranthOS.iso`

## Running

### QEMU with UEFI (recommended)

```bash
make run-qemu
```

Boots the OS in QEMU using OVMF UEFI firmware with 512MB RAM.

### QEMU with SeaBIOS

```bash
make run-seabios
```

Boots using legacy BIOS emulation.

## Project Structure

```
.
├── src/
│   ├── boot/
│   │   └── entry.s          # Assembly entry point
│   ├── kernel/
│   │   └── arch/            # Architecture-specific code
│   │       ├── gdt.c        # Global Descriptor Table
│   │       ├── idt.c        # Interrupt Descriptor Table
│   │       ├── tss.c        # Task State Segment
│   │       ├── pic.c        # Programmable Interrupt Controller
│   │       ├── pit.c        # Programmable Interval Timer
│   │       ├── hal.c        # Hardware Abstraction Layer
│   │       ├── pmm.c        # Physical Memory Manager
│   │       ├── vmm.c        # Virtual Memory Manager
│   │       └── heap.c       # Kernel heap allocator
│   ├── include/
│   │   └── granth/          # Kernel headers
│   ├── kernel.c             # Kernel main entry point
│   └── limine.h             # Limine protocol definitions
├── linker.ld                # Linker script
├── limine.cfg               # Bootloader configuration
└── Makefile                 # Build system

```

## Development

### Clean Build

```bash
make clean
```

Removes all build artifacts from the `out/` directory.

### Compiler Flags

- `-ffreestanding` - Freestanding environment (no standard library)
- `-fno-stack-protector` - Disable stack protection
- `-mno-red-zone` - Disable red zone (required for kernel)
- `-mcmodel=kernel` - Use kernel code model
- `-O2` - Optimization level 2

## Boot Process

1. UEFI firmware loads Limine bootloader
2. Limine reads `limine.cfg` and loads `kernel.elf`
3. Kernel entry point initializes:
   - Serial port for logging
   - Hardware Abstraction Layer
   - Framebuffer graphics
   - Physical memory manager
   - Kernel heap
   - Virtual memory manager
   - GDT, TSS, IDT
   - Timer (100 Hz)
4. Kernel enters idle loop

## Serial Output

The kernel logs to COM1 (0x3F8). In QEMU, serial output is redirected to stdio, so you'll see kernel messages in your terminal.

## License

[Add your license here]

## Contributing

[Add contribution guidelines here]
