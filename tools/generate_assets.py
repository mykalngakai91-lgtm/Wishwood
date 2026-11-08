import os
import struct

def create_bmp(filename, width, height, color):
    r, g, b = color
    # Row size must be padded to a multiple of 4 bytes
    row_bytes = (width * 3 + 3) & ~3
    image_size = row_bytes * height
    file_size = 54 + image_size

    # BITMAPFILEHEADER (14 bytes)
    header = bytearray(b'BM')
    header += struct.pack('<IHHII', file_size, 0, 0, 54, 40)
    # BITMAPINFOHEADER (40 bytes)
    header += struct.pack('<iihhIIiiII', width, height, 1, 24, 0, image_size, 2835, 2835, 0, 0)

    # Pixel data (bottom-up, BGR format)
    pixel_data = bytearray()
    row = bytearray()
    for _ in range(width):
        row += bytes([b, g, r])
    row += bytes(row_bytes - (width * 3))

    for _ in range(height):
        pixel_data += row

    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(pixel_data)
    print(f"[+] Generated asset: {filename}")

if __name__ == '__main__':
    # Generate background and player textures
    create_bmp('assets/bg.bmp', 1920, 1080, (20, 24, 35))
    create_bmp('assets/player.bmp', 64, 64, (0, 230, 138))
