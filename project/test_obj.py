import bpy
import os
import mathutils

blend_filepath = bpy.data.filepath
print("Loaded blend file:", blend_filepath)

filepath = r"C:\Users\k024g\Lesson\2025A\CG2\CG2\project\resources\json\maps\template\template.json"

json_dir = os.path.dirname(filepath)
res_idx = json_dir.find("resources")
if res_idx != -1:
    models_dir = os.path.join(json_dir[:res_idx], "resources", "3dModels")
else:
    models_dir = json_dir

print("models_dir =", models_dir)

if not os.path.exists(models_dir):
    os.makedirs(models_dir)

for obj in bpy.context.scene.objects:
    obj.select_set(False)

for object in bpy.context.scene.objects:
    if object.type != 'MESH':
        continue

    file_name = object.name
    if "file_name" in object and object["file_name"] != "":
        file_name = object["file_name"]
    
    base_name = file_name.split('.')[0]
    if not file_name.endswith(".obj"):
        file_name += ".obj"

    obj_dir = os.path.join(models_dir, base_name)
    if not os.path.exists(obj_dir):
        os.makedirs(obj_dir)

    obj_path = os.path.join(obj_dir, file_name)
    print("Exporting:", obj_path)

    saved_location = object.location.copy()
    saved_rotation = object.rotation_euler.copy()
    saved_scale = object.scale.copy()
    
    object.select_set(True)
    bpy.context.view_layer.objects.active = object

    object.location = mathutils.Vector((0.0, 0.0, 0.0))
    object.rotation_euler = mathutils.Euler((0.0, 0.0, 0.0), 'XYZ')
    object.scale = mathutils.Vector((1.0, 1.0, 1.0))
    
    bpy.context.view_layer.update()

    try:
        if hasattr(bpy.ops.wm, "obj_export"):
            bpy.ops.wm.obj_export(filepath=obj_path, export_selected_objects=True)
        else:
            bpy.ops.export_scene.obj(filepath=obj_path, use_selection=True)
        print("Success:", obj_path)
    except Exception as e:
        print(f"Failed to export {obj_path}: {e}")

    object.location = saved_location
    object.rotation_euler = saved_rotation
    object.scale = saved_scale
    bpy.context.view_layer.update()

    object.select_set(False)
