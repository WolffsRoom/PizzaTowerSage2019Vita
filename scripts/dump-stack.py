import gzip
import struct
import subprocess
import sys

dump_path = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"

with open(dump_path, "rb") as f:
    raw = f.read()

data = gzip.decompress(raw)

# Parse ELF program headers
e_phoff, e_phnum = struct.unpack("<II", data[28:36])[0], struct.unpack("<H", data[44:46])[0]

sp = 0x81c000e0
stack_data = None

for i in range(e_phnum):
    ph = data[e_phoff + i*32 : e_phoff + (i+1)*32]
    p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack("<IIIIIIII", ph)
    if p_vaddr <= sp < p_vaddr + p_memsz:
        stack_offset_in_seg = sp - p_vaddr
        stack_data = data[p_offset + stack_offset_in_seg : p_offset + min(p_filesz, stack_offset_in_seg + 8192)]
        break

if stack_data:
    words = struct.unpack(f"<{len(stack_data)//4}I", stack_data[:len(stack_data)//4 * 4])
    code_ptrs = []
    for idx, w in enumerate(words):
        if 0x81000000 <= w <= 0x81220000:
            code_ptrs.append((idx * 4, sp + idx * 4, w))

    print(f"Found {len(code_ptrs)} code pointers on stack:")
    for off, addr, ptr in code_ptrs[:40]:
        print(f"  +0x{off:x} ({hex(addr)}): {hex(ptr)}")
