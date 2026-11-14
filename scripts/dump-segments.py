import gzip, struct, sys
p = sys.argv[1]
raw = open(p, 'rb').read()
try:
    d = gzip.decompress(raw)
except Exception:
    d = raw
phoff = struct.unpack('<I', d[28:32])[0]
phnum = struct.unpack('<H', d[44:46])[0]
for i in range(phnum):
    t, off, va, pa, fs, ms, fl, al = struct.unpack('<8I', d[phoff+i*32:phoff+i*32+32])
    print(f"ph{i} type={t} off={off:#x} va={va:#010x} filesz={fs:#x} memsz={ms:#x} flags={fl:#x}")
# search module name strings and dump nearby u32 values that look like 0x81xxxxxx
idx = d.find(b'PTSAGE')
print('PTSAGE at', idx)
for name in [b'pizzatowersage', b'eboot', b'PTSAGE001']:
    j = 0
    while True:
        j = d.find(name, j)
        if j < 0: break
        vals = struct.unpack('<32I', d[j-64:j+64])
        print(name, hex(j), [hex(v) for v in vals if 0x81000000 <= v < 0x82000000])
        j += 1

