import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    f.seek(8)
    strg_data = None
    sprt_data = None
    tpag_data = None
    while True:
        hdr = f.read(8)
        if len(hdr) < 8: break
        cname = hdr[:4].decode('ascii', errors='ignore')
        csize = struct.unpack('<I', hdr[4:8])[0]
        cpos = f.tell()
        if cname == 'STRG':
            strg_data = f.read(csize)
            f.seek(cpos)
        elif cname == 'SPRT':
            sprt_data = f.read(csize)
            f.seek(cpos)
        elif cname == 'TPAG':
            tpag_data = f.read(csize)
            f.seek(cpos)
        f.seek(cpos + csize)

    # Let's map string offsets to strings
    # STRG chunk: count, ptrs...
    scount = struct.unpack('<I', strg_data[:4])[0]
    sptrs = struct.unpack(f'<{scount}I', strg_data[4:4+scount*4])
    
    # We can find string offsets
    strings_by_offset = {}
    for p in sptrs:
        slen = struct.unpack('<I', strg_data[p - (f.seek(0, 1) - len(strg_data)):])[0] if False else None

    # Easier: just search for b'spr_press' and b'spr_gamepadbuttons' in strg_data
    for target in [b'spr_press', b'spr_gamepadbuttons', b'spr_pressstart']:
        pos = strg_data.find(target)
        if pos != -1:
            print(f'Found string {target} at {pos}')

