import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    f.seek(8)
    while True:
        hdr = f.read(8)
        if len(hdr) < 8: break
        cname = hdr[:4].decode('ascii', errors='ignore')
        csize = struct.unpack('<I', hdr[4:8])[0]
        cpos = f.tell()
        if cname == 'SHDR':
            count = struct.unpack('<I', f.read(4))[0]
            ptrs = struct.unpack(f'<{count}I', f.read(count * 4))
            print(f'Found SHDR with {count} shaders:')
            for idx, p in enumerate(ptrs):
                if p == 0:
                    print(f'  {idx}: <null>')
                    continue
                f.seek(p)
                name_ptr = struct.unpack('<I', f.read(4))[0]
                stype = struct.unpack('<I', f.read(4))[0]
                es_v = struct.unpack('<I', f.read(4))[0]
                es_f = struct.unpack('<I', f.read(4))[0]
                
                # In GMS, name_ptr is the offset of the string!
                f.seek(name_ptr)
                slen = struct.unpack('<I', f.read(4))[0]
                sname = f.read(slen).decode('ascii', errors='ignore')
                print(f'  {idx}: {sname} (type={stype}, es_v={es_v}, es_f={es_f})')
                
                if es_f != 0:
                    f.seek(es_f)
                    flen = struct.unpack('<I', f.read(4))[0]
                    fsrc = f.read(flen).decode('ascii', errors='ignore')
                    print(f'     Fragment source:\n{fsrc}\n')
            break
        f.seek(cpos + csize)

