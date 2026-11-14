import gzip
import struct

dump_path = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"

with open(dump_path, "rb") as f:
    raw = f.read()

data = gzip.decompress(raw)

# Parse ELF notes
e_phoff, e_phnum = struct.unpack("<II", data[28:36])[0], struct.unpack("<H", data[44:46])[0]

for i in range(e_phnum):
    ph = data[e_phoff + i*32 : e_phoff + (i+1)*32]
    p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack("<IIIIIIII", ph)
    if p_type == 4:
        noff = p_offset
        nend = p_offset + p_filesz
        while noff < nend:
            namesz, descsz, ntype = struct.unpack("<III", data[noff:noff+12])
            name = data[noff+12 : noff+12+namesz].rstrip(b"\x00").decode("latin1", "ignore")
            dpos = (noff + 12 + namesz + 3) & ~3
            desc = data[dpos : dpos + descsz]
            if name in ("COREFILE_INFO", "SUMMARY_INFO", "BUILD_VER_INFO", "APP_INFO"):
                print(f"=== {name} ===")
                words = [hex(w) for w in struct.unpack(f"<{len(desc)//4}I", desc[:len(desc)//4*4])]
                print(words)
            noff = (dpos + descsz + 3) & ~3

