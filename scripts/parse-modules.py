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
            if name == "MODULE_INFO":
                print(f"MODULE_INFO descsz={descsz}")
                # Print module entries
                pos = 0
                while pos + 64 <= descsz:
                    mod_hdr = struct.unpack("<IIII", desc[pos:pos+16])
                    mod_name = desc[pos+16:pos+48].rstrip(b"\x00").decode("latin1", "ignore")
                    text_addr = struct.unpack("<I", desc[pos+48:pos+52])[0]
                    text_size = struct.unpack("<I", desc[pos+52:pos+56])[0]
                    data_addr = struct.unpack("<I", desc[pos+56:pos+60])[0]
                    print(f"  Mod '{mod_name}' text={hex(text_addr)} size={hex(text_size)} data={hex(data_addr)}")
                    pos += 0x60 # or typical module info entry size
            noff = (dpos + descsz + 3) & ~3

