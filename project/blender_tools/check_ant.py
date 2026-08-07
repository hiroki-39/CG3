import bpy
import sys

try:
    rna_type = bpy.ops.mesh.landscape_add.get_rna_type()
    props = rna_type.properties.keys()
    print("\n--- ANT_PROPS_START ---")
    for p in props:
        print(p)
    print("--- ANT_PROPS_END ---\n")
except Exception as e:
    print("\n--- ERROR ---")
    print(e)

sys.exit(0)
