import gzip
import struct

dump_path = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"

with open(dump_path, "rb") as f:
    raw = f.read()

data = gzip.decompress(raw)

# Find THREAD_REG_INFO
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
            if name == "THREAD_REG_INFO":
                print(f"THREAD_REG_INFO len={len(desc)}")
                # Each thread reg info has thread ID and registers
                # In Vita coredump, each entry has struct SceCoredumpThreadRegInfo
                # Let's inspect entries
                pos = 0
                while pos < len(desc):
                    header = struct.unpack("<IIII", desc[pos:pos+16])
                    print("Thread entry header:", [hex(h) for h in header])
                    entry_sz = header[2] # or header[0]
                    if entry_sz == 0: break
                    regs = struct.unpack("<16I", desc[pos+16:pos+16+64])
                    print("Regs (r0-r15):", [hex(r) for r in regs])
                    pos += 16 + 64 + 32 # advance
            if name == "COREFILE_INFO":
                print("COREFILE_INFO:", [hex(x) for x in struct.unpack(f"<{len(desc)//4}I", desc)])
            if name == "META_DATA_INFO":
                print("META_DATA_INFO:", desc[:200])
            noff = (dpos + descsz + 3) & ~3

