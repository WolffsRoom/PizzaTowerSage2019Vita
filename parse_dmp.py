import gzip, struct

dmp = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\Crash Mainmenu\psp2core-1794269507-0x00001334e9-eboot.bin.psp2dmp"
with gzip.open(dmp, "rb") as f:
    decomp = f.read()

print("Decompressed length:", len(decomp))
e_phoff = struct.unpack("<I", decomp[28:32])[0]
e_phnum = struct.unpack("<H", decomp[44:46])[0]
print(f"phoff: {e_phoff}, phnum: {e_phnum}")

for i in range(e_phnum):
    ph = decomp[e_phoff + i*32 : e_phoff + (i+1)*32]
    p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack("<IIIIIIII", ph)
    if p_type == 4: # PT_NOTE
        note = decomp[p_offset : p_offset + p_filesz]
        idx = 0
        while idx < len(note):
            namesz, descsz, ntype = struct.unpack("<III", note[idx:idx+12])
            idx += 12
            name = note[idx:idx+namesz]
            idx = (idx + namesz + 3) & ~3
            desc = note[idx:idx+descsz]
            idx = (idx + descsz + 3) & ~3
            name_str = name.decode("latin1", errors="ignore").strip("\x00")
            print(f"Note type 0x{ntype:x}, name: {name_str}, descsz: {descsz}")
            if ntype in (0x102a, 0x1031):
                text = desc.decode("latin1", errors="ignore")
                print(f"TTY note {hex(ntype)}:\n{text}")
            elif ntype == 0x1004:
                # Thread reg info
                # Let's inspect all fields of THREAD_REG_INFO
                words = [struct.unpack("<I", desc[k:k+4])[0] for k in range(0, descsz, 4)]
                print("THREAD_REG_INFO words:")
                for row in range(0, len(words), 8):
                    print(f"  {row*4:03x}: " + " ".join(f"{w:08x}" for w in words[row:row+8]))