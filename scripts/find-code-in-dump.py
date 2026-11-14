import gzip, struct, sys
p = r"C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\FPS\psp2core-1794282141-0x0000ac34d9-eboot.bin.psp2dmp"
d = gzip.decompress(open(p, 'rb').read())
phoff = struct.unpack('<I', d[28:32])[0]
phnum = struct.unpack('<H', d[44:46])[0]
sp = 0x81bff740
for i in range(phnum):
    t, off, va, pa, fs, ms, fl, al = struct.unpack('<8I', d[phoff+i*32:phoff+i*32+32])
    if t == 1 and va <= sp < va + fs:
        base = off + (sp - va)
        # Search for string pointers or small integers that could be codeId
        for k in range(0, 0x1000, 4):
            v = struct.unpack('<I', d[base+k:base+k+4])[0]
            # check if v points to a string in ram
            # print suspicious values
            pass
        # Let's search entire decompressed dump for "gml_Object_" strings or font_add_sprite calls
        print("Stack found in segment", i)

