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
                    print(f'Sprite {i}: {sname}')
                    # Read texture item pointers
                    f.seek(p + 52)
                    check = struct.unpack('<i', f.read(4))[0]
                    if check == -1:
                        sVer, sType = struct.unpack('<II', f.read(8))
                        if sType == 0:
                            f.seek(8, 1) # speed, speedType
                            if sVer >= 2: f.seek(4, 1)
                            if sVer >= 3: f.seek(4, 1)
                            check = struct.unpack('<I', f.read(4))[0]
                    tcount = check
                    print(f'  textureCount: {tcount}')
                    for t in range(tcount):
                        tpag_ptr = struct.unpack('<I', f.read(4))[0]
                        cur_t = f.tell()
                        f.seek(tpag_ptr)
                        tx, ty, tw, th = struct.unpack('<HHHH', f.read(8))
                        f.seek(tpag_ptr + 20)
                        page = struct.unpack('<H', f.read(2))[0]
                        print(f'    frame {t}: page {page} at ({tx}, {ty}, {tw}, {th})')
                        f.seek(cur_t)
            break
        f.seek(cpos + csize)

