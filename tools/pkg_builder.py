import os
import sys
import struct

def build_pkg(gp4_path, output_pkg):
    print(f"[*] Packaging full game PKG from {gp4_path}...")
    
    files_to_pack = {}

    # Core system binaries & metadata
    if os.path.exists("PARAM.SFO"):
        files_to_pack["sce_sys/param.sfo"] = open("PARAM.SFO", "rb").read()
    if os.path.exists("eboot.bin"):
        files_to_pack["eboot.bin"] = open("eboot.bin", "rb").read()

    # Recursively pack all generated assets
    if os.path.exists("assets"):
        for root, _, files in os.walk("assets"):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, ".").replace("\\", "/")
                files_to_pack[rel_path] = open(full_path, "rb").read()
                print(f"[+] Packed asset: {rel_path}")

    # Construct PKG Header (Fake PKG binary layout)
    header = bytearray(0x2000)
    header[0:4] = b'\x7fPKG'
    
    offset = 0x2000
    entry_index = 0
    payload = bytearray()

    for rel_path, data in files_to_pack.items():
        data_len = len(data)
        struct.pack_into('>II', header, 0x10 + (entry_index * 8), data_len, offset)
        payload.extend(data)
        offset += data_len
        entry_index += 1

    with open(output_pkg, "wb") as f:
        f.write(header)
        f.write(payload)
        
    print(f"[+] Successfully generated full package: {output_pkg} ({os.path.getsize(output_pkg)} bytes)")

if __name__ == "__main__":
    build_pkg("project.gp4", "WISH30300.pkg")
