import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    data = f.read()

# Find CODE chunk
pos = 8
chunks = {}
while pos < len(data) - 8:
    chunk_name = data[pos:pos+4].decode('latin1', 'ignore')
    chunk_size = struct.unpack('<I', data[pos+4:pos+8])[0]
    chunks[chunk_name] = (pos + 8, chunk_size)
    pos += 8 + chunk_size

code_off, code_sz = chunks['CODE']
count = struct.unpack('<I', data[code_off:code_off+4])[0]
ptrs = struct.unpack(f'<{count}I', data[code_off+4:code_off+4+count*4])

for p in ptrs:
    name_ptr = struct.unpack('<I', data[p:p+4])[0]
    slen = struct.unpack('<I', data[name_ptr:name_ptr+4])[0]
    cname = data[name_ptr+4:name_ptr+4+slen].decode('latin1', 'ignore')
    if 'titlecutscene' in cname or 'Scooter' in cname:
        code_length = struct.unpack('<I', data[p+4:p+8])[0]
        print(f"Code entry: {cname}, length={code_length}")

