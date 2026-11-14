import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    f.seek(8)
    while True:
        hdr = f.read(8)
        if len(hdr) < 8: break
        cname = hdr[:4].decode('ascii', errors='ignore')
        csize = struct.unpack('<I', hdr[4:8])[0]
        cpos = f.tell()
        if cname == 'SPRT':
            count = struct.unpack('<I', f.read(4))[0]
            ptrs = struct.unpack(f'<{count}I', f.read(count * 4))
            for i, p in enumerate(ptrs):
                if p == 0: continue
                f.seek(p)
                np = struct.unpack('<I', f.read(4))[0]
                cur = f.tell()
                f.seek(np)
                slen = struct.unpack('<I', f.read(4))[0]
                sname = f.read(slen).decode('ascii', errors='ignore').split('\x00')[0]
                f.seek(cur)
                if 'pal' in sname.lower():
                    w, h = struct.unpack('<II', f.read(8))
                    print(f'{i}: {sname} ({w}x{h})')
            break
        f.seek(cpos + csize)

