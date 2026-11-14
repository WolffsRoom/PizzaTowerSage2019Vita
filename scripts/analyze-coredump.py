import gzip
import struct

dump_path = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"

with open(dump_path, "rb") as f:
    raw = f.read()

data = gzip.decompress(raw)
print(f"Decompressed dump size: {len(data)} bytes")

# In SceCoredump format, there are ELF headers or dump blocks
print("First 128 bytes:", data[:128])

# Search for PC
for offset in range(0, min(len(data), 0x10000) - 4, 4):
    val = struct.unpack("<I", data[offset:offset+4])[0]
    if val & 0xFFFFFFF0 == 0x812f2530 or val & 0xFFFFFFF0 == 0x002f2530:
        print(f"Found PC {hex(val)} at offset {hex(offset)}")
        regs_start = max(0, offset - 64)
        regs_end = min(len(data), offset + 68)
        vals = [hex(struct.unpack("<I", data[i:i+4])[0]) for i in range(regs_start, regs_end, 4)]
        print(f"Surrounding: {vals}")

# Also search for strings in dump
import re
ascii_strings = re.findall(b"[a-zA-Z0-9_./]{6,}", data)
print("Some strings:", [s.decode('ascii') for s in ascii_strings[:30]])
