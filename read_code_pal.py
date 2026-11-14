import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    data = f.read()

pos = data.find(b'gml_Script_pal_swap_set\x00')
print('pos:', pos)
# Let's search in CODE chunk
code_pos = data.find(b'CODE')
print('CODE pos:', code_pos)

# Let's search for "pal_swap_set" in CODE names
# In butterscotch data_win.c:
# parseCODE: count, ptrs...
cpos = code_pos + 8
count = struct.unpack('<I', data[cpos:cpos+4])[0]
print('code count:', count)
ptrs = struct.unpack(f'<{count}I', data[cpos+4:cpos+4+count*4])

strg_pos = data.find(b'STRG')
def get_str(p):
    slen = struct.unpack('<I', data[p:p+4])[0]
    return data[p+4:p+4+slen].decode('ascii', errors='ignore').split('\x00')[0]

for p in ptrs:
    name_p = struct.unpack('<I', data[p:p+4])[0]
    cname = get_str(name_p)
    if 'pal_swap_set' == cname:
        code_len = struct.unpack('<I', data[p+4:p+8])[0]
        print(f'Found code {cname}, len={code_len}')
        # print first 100 bytes
        # in GMS bytecode: instructions
        break

