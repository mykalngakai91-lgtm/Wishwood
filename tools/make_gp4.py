import sys
import os

def generate_gp4(content_dir, output_gp4):
    gp4_content = '''<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<ps4project fmt="gp4" version="1.0">
  <volume type="pkg_pkg" storage_type="local">
    <volume_info>
      <package_type>full</package_type>
      <title>Wishwood: Kingdom 3030</title>
      <content_id>IV0002-WISH30300_00-0000000000000001</content_id>
      <passcode>e0000000000000000000000000000000</passcode>
    </volume_info>
    <files>
'''
    
    for root, dirs, files in os.walk(content_dir):
        for file in files:
            full_path = os.path.join(root, file)
            rel_path = os.path.relpath(full_path, content_dir).replace('\\', '/')
            gp4_content += f'      <file targ_path="{rel_path}">{rel_path}</file>\n'

    gp4_content += '''    </files>
  </volume>
</ps4project>
'''
    with open(output_gp4, 'w') as f:
        f.write(gp4_content)

if __name__ == '__main__':
    c_dir = sys.argv[1] if len(sys.argv) > 1 else "pkg_root"
    out = sys.argv[2] if len(sys.argv) > 2 else "project.gp4"
    generate_gp4(c_dir, out)
