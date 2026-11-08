import sys
import struct

def create_sfo(title, output_path):
    # Simple PARAM.SFO generator for PS4 Homebrew
    # SFO header format
    magic = b'\x00PSF'
    version = b'\x01\x01\x00\x00'
    key_table_offset = 0x24
    val_table_offset = 0x64
    entries = 1

    # Key: TITLE, Type: UTF8 (2), MaxLen: 0x80, ValLen: len(title)+1, Offset: 0
    header = struct.pack('<4s4sIIII', magic, version, key_table_offset, val_table_offset, entries)
    
    key_offset = 0  # relative to key table
    val_offset = 0  # relative to val table
    
    # We'll write a standard simple SFO structure
    # Key: "TITLE\0"
    key_entry = b'TITLE\x00\x00\x03' # padding
    # Format for index table entry: (key_offset, param_fmt(2:str), val_len, val_maxlen, val_offset)
    # Let's use a robust pre-built binary block for standard homebrew SFO
    sfo_data = bytearray(0x200)
    # Magic & header
    sfo_data[0:4]   = magic
    sfo_data[4:8]   = version
    sfo_data[8:12]  = struct.pack('<I', 0x14) # key table offset
    sfo_data[12:16] = struct.pack('<I', 0x68) # val table offset
    sfo_data[16:20] = struct.pack('<I', 1)    # tables entries count

    # Index table entry 0 (TITLE)
    # offset 0x14: data key offset (4 bytes), param format (2 bytes: 0x0204 = UTF8), val len (4 bytes), val max len (4 bytes), data val offset (4 bytes)
    title_bytes = title.encode('utf-8') + b'\x00'
    val_len = len(title_bytes)
    
    sfo_data[0x20:0x24] = struct.pack('<I', 0)     # offset in key table for "TITLE"
    sfo_data[0x24:0x26] = struct.pack('<H', 0x0204)# data type: utf8
    sfo_data[0x26:0x2a] = struct.pack('<I', val_len)
    sfo_data[0x2a:0x2e] = struct.pack('<I', 0x80)  # max len
    sfo_data[0x2e:0x32] = struct.pack('<I', 0)     # offset in val table

    # Key table (starts at 0x14)
    sfo_data[0x14:0x1A] = b'TITLE\x00'

    # Value table (starts at 0x68)
    sfo_data[0x68:0x68+val_len] = title_bytes

    with open(output_path, 'wb') as f:
        f.write(sfo_data)

if __name__ == '__main__':
    title = sys.argv[1] if len(sys.argv) > 1 else "Wishwood: Kingdom 3030"
    out = sys.argv[2] if len(sys.argv) > 2 else "PARAM.SFO"
    create_sfo(title, out)
