import os
import sys
import struct

def build_pkg(gp4_path, output_pkg):
    print(f"[*] Packaging PS4 PKG from {gp4_path}...")
    
    # Read eboot and param.sfo
    with open("eboot.bin", "rb") as f:
        eboot_data = f.read()
    with open("PARAM.SFO", "rb") as f:
        sfo_data = f.read()

    # Fake PKG Header structure for PS4 homebrew
    # PKG Magic (\x7fPKG)
    header = bytearray(0x2000)
    header[0:4] = b'\x7fPKG'
    
    # Write Entry Offsets & Sizes
    # PARAM.SFO entry
    struct.pack_into('>II', header, 0x10, len(sfo_data), 0x2000)
    # EBOOT.BIN entry
    struct.pack_into('>II', header, 0x18, len(eboot_data), 0x2000 + len(sfo_data))

    with open(output_pkg, "wb") as f:
        f.write(header)
        f.write(sfo_data)
        f.write(eboot_data)
        
    print(f"[+] Successfully generated {output_pkg} ({os.path.getsize(output_pkg)} bytes)")

if __name__ == "__main__":
    build_pkg("project.gp4", "WISH30300.pkg")
