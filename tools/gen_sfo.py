import struct
import sys

def create_sfo(title, output_path):
    magic = b'\x00PSF'
    version = b'\x01\x01\x00\x00'
    key_table_offset = 0x00000028
    val_table_offset = 0x0000002C
    entries = 1

    # Fix: Ensure exactly 6 items are passed to match '<4s4sIIII'
    header = struct.pack('<4s4sIIII', magic, version, key_table_offset, val_table_offset, entries, 0)

    with open(output_path, 'wb') as f:
        f.write(header)
        # Key string 'TITLE\0'
        f.write(b'TITLE\0\0\0')
        # Value string with null terminator
        title_bytes = title.encode('utf-8') + b'\0'
        f.write(title_bytes)

    print(f"PARAM.SFO generated successfully for '{title}'")

if __name__ == '__main__':
    create_sfo("Wishwood: Kingdom 3030", "PARAM.SFO")
