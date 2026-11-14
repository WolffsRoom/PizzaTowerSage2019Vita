import struct

data = open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb').read()

# Let's parse CODE chunk properly:
code_pos = data.find(b'CODE')
code_len = struct.unpack('<I', data[code_pos+4:code_pos+8])[0]
code_count = struct.unpack('<I', data[code_pos+8:code_pos+12])[0]
print(f'CODE count: {code_count}')

# Look in STRG chunk for string offsets
str_add_ext = data.find(b'font_add_sprite_ext\x00')
str_add = data.find(b'font_add_sprite\x00')
print('str_add_ext:', hex(str_add_ext), 'str_add:', hex(str_add))

# Find in FUNC chunk:
func_pos = data.find(b'FUNC')
chunk_len = struct.unpack('<I', data[func_pos+4:func_pos+8])[0]
# WadVersion <= 14 uses 12-byte entries
# WadVersion >= 15 uses SimpleList (count + pointers)
first_val = struct.unpack('<I', data[func_pos+8:func_pos+12])[0]
print('first_val in FUNC:', first_val, hex(first_val))

# Let's find occurrences of str_add_ext in FUNC
for off in range(func_pos+8, func_pos+8+chunk_len, 4):
    v = struct.unpack('<I', data[off:off+4])[0]
    if v == str_add_ext:
        print('Found str_add_ext pointer at func offset', hex(off))
        # nearby entries
        occ, first_addr = struct.unpack('<II', data[off+4:off+12])
        print(f'occ={occ}, first_addr={first_addr:#x}')
        # In GM bytecode, first_addr is the bytecode offset in the first CODE block that calls it!
        # Following addresses form a linked list in the bytecode!
        curr = first_addr
        while curr != 0xFFFFFFFF and curr != 0 and curr < len(data):
            # find which CODE entry contains curr
            # In bytecode, the instruction at curr has the next address in its immediate!
            next_addr = struct.unpack('<i', data[curr+4:curr+8])[0]
            # find code entry
            for c in range(code_count):
                entry_off = struct.unpack('<I', data[code_pos+12+c*4:code_pos+16+c*4])[0]
                body_len = struct.unpack('<I', data[entry_off+4:entry_off+8])[0]
                code_start = entry_off + 16
                if code_start <= curr < code_start + body_len:
                    name_ptr = struct.unpack('<I', data[entry_off:entry_off+4])[0]
                    name = data[name_ptr:].split(b'\x00')[0].decode('latin1')
                    print(f'Called in {name} at offset {curr - code_start:#x}')
                    break
            if next_addr >= 0:
                curr += next_addr
            else:
                break

