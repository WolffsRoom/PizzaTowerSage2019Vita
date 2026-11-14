import struct

with open(r'C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win', 'rb') as f:
    data = f.read()

def find_script(name):
    target = name.encode('ascii')
    idx = 0
    while True:
        pos = data.find(target, idx)
        if pos == -1: break
        idx = pos + len(target)
        # Check if it's in CODE or SCPT chunk
        print(f'Found {name} at {pos}')
        # print surrounding text/bytes
        start = max(0, pos - 20)
        end = min(len(data), pos + 100)
        print('Context:', data[start:end])

find_script('pal_swap_set')

