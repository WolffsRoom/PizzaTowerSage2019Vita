import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    f.seek(8) # exactly after FORM and its size
    shdr_offset = 0
    shdr_size = 0
    while True:
        hdr = f.read(8)
        if len(hdr) < 8: break
        cname = hdr[:4].decode('ascii', errors='ignore')
        csize = struct.unpack('<I', hdr[4:8])[0]
        if cname == 'SHDR':
            shdr_offset = f.tell()
            shdr_size = csize
            break
        f.seek(csize, 1)

    print(f'SHDR at {shdr_offset}, size {shdr_size}')
    f.seek(shdr_offset)
    count = struct.unpack('<I', f.read(4))[0]
    ptrs = struct.unpack(f'<{count}I', f.read(count * 4))
    
    def read_str(p):
        if p == 0: return ""
        cur = f.tell()
        f.seek(p)
        slen = struct.unpack('<I', f.read(4))[0]
        s = f.read(slen).decode('utf-8', errors='ignore')
        f.seek(cur)
        return s

    for i, p in enumerate(ptrs):
        if p == 0:
            print(f'Shader {i}: <null>')
            continue
        f.seek(p)
        name_ptr = struct.unpack('<I', f.read(4))[0]
        stype = struct.unpack('<I', f.read(4))[0]
        es_vert = struct.unpack('<I', f.read(4))[0]
        es_frag = struct.unpack('<I', f.read(4))[0]
        gl_vert = struct.unpack('<I', f.read(4))[0]
        gl_frag = struct.unpack('<I', f.read(4))[0]
        name = read_str(name_ptr)
        print(f'Shader {i}: {name} (type={stype})')
        if 'pal' in name.lower() or 'swap' in name.lower() or 'alpha' in name.lower():
            print(f'--- ES_VERT for {name} ---:\n{read_str(es_vert)}')
            print(f'--- ES_FRAG for {name} ---:\n{read_str(es_frag)}')
