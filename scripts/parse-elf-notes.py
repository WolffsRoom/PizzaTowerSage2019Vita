import gzip
import struct

dump_path = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"

with open(dump_path, "rb") as f:
    raw = f.read()

data = gzip.decompress(raw)

# Parse ELF Header
e_ident = data[:16]
e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags, e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack("<HHIIIIIHHHHHH", data[16:52])
print(f"e_phoff={e_phoff}, e_phnum={e_phnum}")

# Read program headers to find PT_NOTE
for i in range(e_phnum):
    ph = data[e_phoff + i*32 : e_phoff + (i+1)*32]
    p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack("<IIIIIIII", ph)
    if p_type == 4: # PT_NOTE
        print(f"PT_NOTE at offset={hex(p_offset)}, size={hex(p_filesz)}")
        # Parse notes
        noff = p_offset
        nend = p_offset + p_filesz
        while noff < nend:
            namesz, descsz, ntype = struct.unpack("<III", data[noff:noff+12])
            name = data[noff+12 : noff+12+namesz].rstrip(b"\x00").decode("latin1", "ignore")
            # align to 4
            dpos = (noff + 12 + namesz + 3) & ~3
            desc = data[dpos : dpos + descsz]
            print(f"  Note type={hex(ntype)} name='{name}' descsz={descsz}")
            if "THREAD" in name or ntype in (1, 0x1000, 0x1001, 0x1002, 0x1003):
                # Print hex of desc
                # Let's inspect first few words
                words = [hex(w) for w in struct.unpack(f"<{min(len(desc)//4, 32)}I", desc[:min(len(desc), 128)])]
                print(f"    Desc words: {words}")
            noff = (dpos + descsz + 3) & ~3
