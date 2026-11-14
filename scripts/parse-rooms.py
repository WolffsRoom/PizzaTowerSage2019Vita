import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    data = f.read()

# Parse chunks
pos = 8
chunks = {}
while pos < len(data) - 8:
    chunk_name = data[pos:pos+4].decode('latin1', 'ignore')
    chunk_size = struct.unpack('<I', data[pos+4:pos+8])[0]
    chunks[chunk_name] = (pos + 8, chunk_size)
    pos += 8 + chunk_size

print("Found chunks:", list(chunks.keys()))

# Get strings
strg_offset, strg_size = chunks['STRG']
strg_count = struct.unpack('<I', data[strg_offset:strg_offset+4])[0]
strg_ptrs = struct.unpack(f'<{strg_count}I', data[strg_offset+4:strg_offset+4+strg_count*4])

strings = {}
for idx, ptr in enumerate(strg_ptrs):
    slen = struct.unpack('<I', data[ptr:ptr+4])[0]
    sval = data[ptr+4:ptr+4+slen].decode('latin1', 'ignore')
    strings[ptr] = sval

# Get room names
room_offset, room_size = chunks['ROOM']
room_count = struct.unpack('<I', data[room_offset:room_offset+4])[0]
room_ptrs = struct.unpack(f'<{room_count}I', data[room_offset+4:room_offset+4+room_count*4])

for r_idx, r_ptr in enumerate(room_ptrs):
    r_name_ptr = struct.unpack('<I', data[r_ptr:r_ptr+4])[0]
    slen = struct.unpack('<I', data[r_name_ptr:r_name_ptr+4])[0]
    r_name = data[r_name_ptr+4:r_name_ptr+4+slen].decode('latin1', 'ignore')
    if "Scooter" in r_name or "Realtitlescreen" in r_name or "Titlescreen" in r_name or "hub" in r_name:
        print(f"Room [{r_idx}] = '{r_name}'")
