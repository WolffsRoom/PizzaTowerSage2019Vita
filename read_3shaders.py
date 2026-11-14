import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    f.seek(8)
    shdr_data = None
    while True:
        hdr = f.read(8)
        if len(hdr) < 8: break
        cname = hdr[:4].decode('ascii', errors='ignore')
        csize = struct.unpack('<I', hdr[4:8])[0]
        cpos = f.tell()
        if cname == 'SHDR':
            shdr_data = f.read(csize)
            f.seek(cpos)
            break
        f.seek(cpos + csize)

    shdr_count = struct.unpack('<I', shdr_data[:4])[0]
    shdr_ptrs = struct.unpack(f'<{shdr_count}I', shdr_data[4:4+shdr_count*4])
    print(f'SHDR count: {shdr_count}')
    
    def get_str(ptr):
        f.seek(ptr)
        slen = struct.unpack('<I', f.read(4))[0]
        # In GMS, string length includes null terminator or doesn't? Let's read slen bytes and strip null
        raw = f.read(slen)
        if b'\x00' in raw:
            raw = raw[:raw.index(b'\x00')]
        return raw.decode('utf-8', errors='ignore')

    for i, p in enumerate(shdr_ptrs):
        if p == 0:
            print(f'Shader {i}: null')
            continue
        f.seek(p)
        name_p = struct.unpack('<I', f.read(4))[0]
        stype = struct.unpack('<I', f.read(4))[0]
        es_v = struct.unpack('<I', f.read(4))[0]
        es_f = struct.unpack('<I', f.read(4))[0]
        gl_v = struct.unpack('<I', f.read(4))[0]
        gl_f = struct.unpack('<I', f.read(4))[0]
        
        name = get_str(name_p)
        print(f'=== Shader {i}: "{name}" ===')
        print(f'type: {stype}')
        if es_v: print(f'--- ES Vertex ---\n{get_str(es_v)}\n')
        if es_f: print(f'--- ES Fragment ---\n{get_str(es_f)}\n')

