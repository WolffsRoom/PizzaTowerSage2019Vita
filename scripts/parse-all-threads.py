import gzip
import struct

dump_path = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"

with open(dump_path, "rb") as f:
    raw = f.read()

data = gzip.decompress(raw)

# Parse ELF notes
e_phoff, e_phnum = struct.unpack("<II", data[28:36])[0], struct.unpack("<H", data[44:46])[0]

threads = {}

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
            if name == "THREAD_INFO":
                # Each thread info is struct
                # Let's inspect size of entries
                print(f"THREAD_INFO descsz={descsz}")
                pos = 0
                while pos + 0x100 <= descsz:
                    t_hdr = struct.unpack("<IIII", desc[pos:pos+16])
                    t_name = desc[pos+16:pos+48].rstrip(b"\x00").decode("latin1", "ignore")
                    th_id = t_hdr[3]
                    print(f"  Thread id={hex(th_id)} name='{t_name}'")
                    pos += 0x1c8 # typical thread info size
            if name == "THREAD_REG_INFO":
                print(f"THREAD_REG_INFO descsz={descsz}")
                pos = 0
                idx = 0
                while pos + 0x80 <= descsz:
                    hdr = struct.unpack("<IIII", desc[pos:pos+16])
                    th_id = hdr[3]
                    regs = struct.unpack("<16I", desc[pos+16:pos+16+64])
                    cpsr = struct.unpack("<I", desc[pos+80:pos+84])[0] if pos+84<=descsz else 0
                    print(f"  Thread [{idx}] id={hex(th_id)} PC={hex(regs[15])} LR={hex(regs[14])} SP={hex(regs[13])} CPSR={hex(cpsr)}")
                    pos += 0x178 # SceCoredumpThreadRegInfo size is usually 0x178
                    idx += 1
            noff = (dpos + descsz + 3) & ~3

