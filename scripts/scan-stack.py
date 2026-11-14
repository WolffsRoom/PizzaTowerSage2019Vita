import gzip, struct, sys
p, sp = sys.argv[1], int(sys.argv[2], 16)
bias = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x42000
words = int(sys.argv[4]) if len(sys.argv) > 4 else 512
d = gzip.decompress(open(p, 'rb').read())
phoff = struct.unpack('<I', d[28:32])[0]
phnum = struct.unpack('<H', d[44:46])[0]
for i in range(phnum):
    t, off, va, pa, fs, ms, fl, al = struct.unpack('<8I', d[phoff+i*32:phoff+i*32+32])
    if t == 1 and va <= sp < va + fs:
        base = off + (sp - va)
        n = min(words, (va + fs - sp) // 4)
        for k in range(n):
            v = struct.unpack('<I', d[base+k*4:base+k*4+4])[0]
            if (v & 1) and 0x81042000 <= v < 0x81042000 + 0x21ef68:
                print(f"sp+{k*4:#06x} {v:#010x} elf={v-bias:#010x}")
        break

