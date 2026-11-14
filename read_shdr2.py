import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    f.seek(8)
    strg_base = 0
    strg_data = None
    shdr_data = None
    while True:
        hdr = f.read(8)
        if len(hdr) < 8: break
        cname = hdr[:4].decode('ascii', errors='ignore')
        csize = struct.unpack('<I', hdr[4:8])[0]
        cpos = f.tell()
        if cname == 'STRG':
            strg_base = cpos
            strg_data = f.read(csize)
            f.seek(cpos)
        elif cname == 'SHDR':
            shdr_data = f.read(csize)
            f.seek(cpos)
        f.seek(cpos + csize)

    if strg_data and shdr_data:
        # strgBufferBase in butterscotch is the position after readPointerTable in STRG
        # In STRG: count (4 bytes), then ptrs (count * 4 bytes)
        scount = struct.unpack('<I', strg_data[:4])[0]
        strg_first_str = strg_base + 4 + scount * 4
        # But wait! offset in readStringPtr is offset - strgBufferBase, or is offset an absolute pointer in data.win?
        # Let's check the first pointer in STRG
        first_ptr = struct.unpack('<I', strg_data[4:8])[0]
        print(f'STRG base: {strg_base}, scount: {scount}, first_ptr: {first_ptr}')
        
        # Let's check SHDR
        shdr_count = struct.unpack('<I', shdr_data[:4])[0]
        shdr_ptrs = struct.unpack(f'<{shdr_count}I', shdr_data[4:4+shdr_count*4])
        print(f'SHDR count: {shdr_count}')
        for i, p in enumerate(shdr_ptrs):
            if p == 0: continue
            f.seek(p)
            name_offset = struct.unpack('<I', f.read(4))[0]
            stype = struct.unpack('<I', f.read(4))[0]
            es_v_off = struct.unpack('<I', f.read(4))[0]
            es_f_off = struct.unpack('<I', f.read(4))[0]
            
            # string at name_offset
            f.seek(name_offset)
            slen = struct.unpack('<I', f.read(4))[0]
            sname = f.read(slen).decode('ascii', errors='ignore')
            print(f'Shader {i}: "{sname}" (type={stype})')
            
            if es_v_off:
                f.seek(es_v_off)
                vlen = struct.unpack('<I', f.read(4))[0]
                vsrc = f.read(vlen).decode('ascii', errors='ignore')
                print(f'  Vertex:\n{vsrc}')
            if es_f_off:
                f.seek(es_f_off)
                flen = struct.unpack('<I', f.read(4))[0]
                fsrc = f.read(flen).decode('ascii', errors='ignore')
                print(f'  Fragment:\n{fsrc}')

