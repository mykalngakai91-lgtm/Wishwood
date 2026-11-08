import os

gp4_content = """<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<uo_project version="1.00">
  <header>
    <content_id>IV0000-WISH30300_00-WISHWOOD30300000</content_id>
    <title_id>WISH30300</title_id>
    <volume_type>pkg_ps4_app</volume_type>
  </header>
  <files count="2">
    <file target_path="eboot.bin" source_path="eboot.bin" />
    <file target_path="sce_sys/param.sfo" source_path="PARAM.SFO" />
  </files>
</uo_project>
"""

with open("project.gp4", "w") as f:
    f.write(gp4_content)

print("project.gp4 created successfully")
