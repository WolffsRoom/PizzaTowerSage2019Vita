with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    data = f.read()
    for name in [b'pal_swap_set', b'pal_swap_init_system', b'pal_swap_draw_palette', b'palette_texture', b'texel_size', b'palette_UVs', b'palette_index']:
        print(name.decode(), data.count(name))

