# -*- coding: utf-8 -*-
import bpy
import math
import bpy_extras
import gpu
import gpu_extras.batch
import copy
import mathutils
import json

bl_info = {
    "name": "レベルエディタ",
    "author": "Hiroki kato",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "",
    "description": "レベルエディタ",
    "category": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object"
}

#コライダー描画
class DrawCollider:
    # 描画ハンドル
    handle = None

    # 3Dビューに登録する描画関数
    def draw_collider():
        # 頂点データとインデックスデータ
        vertices = {"pos": []}
        indices = []

        offsets = [
            [-0.5, -0.5, -0.5], [+0.5, -0.5, -0.5], [-0.5, +0.5, -0.5], [+0.5, +0.5, -0.5],
            [-0.5, -0.5, +0.5], [+0.5, -0.5, +0.5], [-0.5, +0.5, +0.5], [+0.5, +0.5, +0.5]
        ]

        for object in bpy.context.scene.objects:
            if not "collider" in object:
                continue

            collider_type = object.get("collider_type", object.get("collider", "BOX"))
            center = mathutils.Vector(object.get("collider_center", (0,0,0)))
            start = len(vertices["pos"])

            if collider_type == 'SPHERE':
                radius = object.get("collider_radius", 1.0)
                segments = 16
                for plane in range(3):
                    for i in range(segments):
                        angle = (i / segments) * 2 * math.pi
                        x = math.cos(angle) * radius
                        y = math.sin(angle) * radius
                        pos = copy.copy(center)
                        if plane == 0:
                            pos[0] += x; pos[1] += y
                        elif plane == 1:
                            pos[1] += x; pos[2] += y
                        else:
                            pos[2] += x; pos[0] += y
                        
                        pos = object.matrix_world @ pos
                        vertices["pos"].append(pos)
                        
                        idx = start + plane * segments + i
                        next_idx = start + plane * segments + ((i + 1) % segments)
                        indices.append([idx, next_idx])
            
            else: # AABB, OBB, BOX
                size = mathutils.Vector(object.get("collider_size", (2,2,2)))
                for offset in offsets:
                    pos = copy.copy(center)
                    pos[0] += offset[0] * size[0]
                    pos[1] += offset[1] * size[1]
                    pos[2] += offset[2] * size[2]
                    
                    if collider_type == 'AABB':
                        # AABBは回転させず、スケールと平行移動のみ適用
                        mat_trans = mathutils.Matrix.Translation(object.location)
                        mat_scale = mathutils.Matrix.Scale(object.scale[0], 4, (1,0,0)) @ mathutils.Matrix.Scale(object.scale[1], 4, (0,1,0)) @ mathutils.Matrix.Scale(object.scale[2], 4, (0,0,1))
                        pos = (mat_trans @ mat_scale) @ pos
                    else: # OBB
                        pos = object.matrix_world @ pos
                        
                    vertices["pos"].append(pos)

                indices.extend([
                    [start+0, start+1], [start+2, start+3], [start+0, start+2], [start+1, start+3],
                    [start+4, start+5], [start+6, start+7], [start+4, start+6], [start+5, start+7],
                    [start+0, start+4], [start+1, start+5], [start+2, start+6], [start+3, start+7]
                ])

        if len(vertices["pos"]) == 0:
            return

        shader = gpu.shader.from_builtin('UNIFORM_COLOR')
        batch = gpu_extras.batch.batch_for_shader(shader, 'LINES', vertices, indices=indices)
        color = [0.5, 1.0, 1.0, 1.0]
        shader.bind()
        shader.uniform_float("color", color)
        batch.draw(shader)


#オペレータ　頂点を伸ばす
class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    bl_idname = "myaddon.stretch_vertex"
    bl_label = "頂点を伸ばす"
    bl_description = "頂点座標を引っ張って伸ばします"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        #ここに頂点を伸ばす処理を書く
        bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
        print("頂点を伸ばす処理が実行されました。")
        return {'FINISHED'}

#オペレータ　ICO球生成
class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    bl_idname = "myaddon.create_ico_sphere"
    bl_label = "ICO球生成"
    bl_description = "ICO球を生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        #ここにICO球を生成する処理を書く
        bpy.ops.mesh.primitive_ico_sphere_add()
        print("ICO球が生成されました。")
        return {'FINISHED'}

#オペレータ　Fighter（敵）生成
class MYADDON_OT_create_fighter(bpy.types.Operator):
    bl_idname = "myaddon.create_fighter"
    bl_label = "敵(Fighter)生成"
    bl_description = "敵(Fighter)の配置用ダミーを生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        empty_obj = bpy.data.objects.new("Enemy_Fighter", None)
        empty_obj.empty_display_type = 'CUBE'
        empty_obj.empty_display_size = 2.0
        empty_obj.location = context.scene.cursor.location
        empty_obj["file_name"] = "Fighter"
        empty_obj.is_enemy_flag = True
        empty_obj["is_enemy"] = True
        empty_obj["spawn_progress"] = 0.0
        context.scene.collection.objects.link(empty_obj)
        context.view_layer.objects.active = empty_obj
        empty_obj.select_set(True)
        print("敵(Fighter)ダミーを生成しました。")
        return {'FINISHED'}

#オペレータ　Asteroid（障害物）生成
class MYADDON_OT_create_asteroid(bpy.types.Operator):
    bl_idname = "myaddon.create_asteroid"
    bl_label = "障害物(Asteroid)生成"
    bl_description = "障害物(Asteroid)の配置用ダミーを生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        empty_obj = bpy.data.objects.new("Obstacle_Asteroid", None)
        empty_obj.empty_display_type = 'SPHERE'
        empty_obj.empty_display_size = 3.0
        empty_obj.location = context.scene.cursor.location
        empty_obj["file_name"] = "Asteroid"
        empty_obj["is_obstacle"] = True
        context.scene.collection.objects.link(empty_obj)
        context.view_layer.objects.active = empty_obj
        empty_obj.select_set(True)
        print("障害物(Asteroid)ダミーを生成しました。")
        return {'FINISHED'}

#オペレータ　ゲームカメラ生成
class MYADDON_OT_create_game_camera(bpy.types.Operator):
    bl_idname = "myaddon.create_game_camera"
    bl_label = "ゲームカメラ生成"
    bl_description = "ゲーム内の視野角(FOV)を再現したカメラを生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        camera_data = bpy.data.cameras.new(name="GameCameraData")
        camera_data.lens_unit = 'FOV'
        camera_data.angle = math.radians(45.0) # ゲームのFOVに合わせて45度に設定
        camera_obj = bpy.data.objects.new("GameCamera", camera_data)
        camera_obj.location = context.scene.cursor.location
        camera_obj.rotation_euler = (math.radians(90), 0, 0) # 真っ直ぐ前を向くように(Y軸プラス方向)
        context.scene.collection.objects.link(camera_obj)
        context.view_layer.objects.active = camera_obj
        camera_obj.select_set(True)
        print("ゲームカメラを生成しました。")
        return {'FINISHED'}

#オペレータ　カスタムプロパティ['file_name']追加
class MYADDON_OT_add_filename(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_filename"
    bl_label = "FileName 追加"
    bl_description = "オブジェクトにカスタムプロパティ['file_name']を追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        #[file_name]プロパティを追加
        context.object["file_name"] = ""

        return {'FINISHED'}

#オペレータ　カスタムプロパティ['collider']追加
class MYADDON_OT_add_collider(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_collider"
    bl_label = "コライダー 追加"
    bl_description = "['collider']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        context.object["collider"] = "SPHERE"
        context.object["collider_type"] = "SPHERE"
        context.object["collider_center"] = mathutils.Vector((0.0, 0.0, 0.0))
        context.object["collider_size"] = mathutils.Vector((2.0, 2.0, 2.0))
        context.object["collider_radius"] = 1.0

        return {'FINISHED'}

#パネル ファイル名
class OBJECT_PT_file_name(bpy.types.Panel):
    """オブジェクトのファイルネームパネル"""
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "ファイル名"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        #パネルに項目を追加
        if "file_name" in context.object:
            #既にプロパティがあれば、プロパティを表示
            self.layout.prop(context.object, '["file_name"]', text= self.bl_label)
        else:
            #プロパティがなければ、プロパティ追加のオペレータを表示
            self.layout.operator(MYADDON_OT_add_filename.bl_idname)

#パネル　コライダー
class OBJECT_PT_collider(bpy.types.Panel):
    """オブジェクトのコライダーパネル"""
    bl_idname = "OBJECT_PT_collider"
    bl_label = "コライダー設定"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        #パネルに項目を追加
        if "collider" in context.object:
            # カスタムプロパティをUIから直接変更できるようにする
            self.layout.prop(context.object, '["collider_type"]', text="形状 (SPHERE/BOX)")
            self.layout.prop(context.object, '["collider_center"]', text="中心のズレ")
            
            # タイプに応じたプロパティの表示
            c_type = context.object.get("collider_type", "")
            if c_type == 'SPHERE':
                self.layout.prop(context.object, '["collider_radius"]', text="半径")
            else:
                self.layout.prop(context.object, '["collider_size"]', text="サイズ")
        else:
            #プロパティがなければ、プロパティ追加のオペレータを表示
            self.layout.operator(MYADDON_OT_add_collider.bl_idname)

#オペレータ カスタムプロパティ['is_destructible']追加
class MYADDON_OT_add_destructible(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_destructible"
    bl_label = "Destructibleフラグ 追加"
    bl_description = "['is_destructible']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        context.object["is_destructible"] = True
        return {'FINISHED'}

#パネル Destructible
class OBJECT_PT_destructible(bpy.types.Panel):
    """オブジェクトの破壊フラグパネル"""
    bl_idname = "OBJECT_PT_destructible"
    bl_label = "破壊可能フラグ"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        if "is_destructible" in context.object:
            self.layout.prop(context.object, '["is_destructible"]', text="破壊可能（チェックで壊れる）")
        else:
            self.layout.operator(MYADDON_OT_add_destructible.bl_idname)

#パネル Enemy Settings
class OBJECT_PT_enemy_settings(bpy.types.Panel):
    """オブジェクトの敵設定パネル"""
    bl_idname = "OBJECT_PT_enemy_settings"
    bl_label = "敵設定 (Enemy Settings)"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        obj = context.object
        self.layout.prop(obj, "is_enemy_flag")
        if obj.is_enemy_flag:
            self.layout.prop(obj, "enemy_type")
            self.layout.prop(obj, "enemy_target")
            self.layout.prop(obj, "enemy_max_y")
            self.layout.prop(obj, "enemy_min_y")
            self.layout.prop(obj, "enemy_formation_id")

#オペレータ　シーン出力
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportします"
    #出力するファイルの拡張子
    filename_ext = ".json"

    def parse_scene_recursive(self, file, object, level):
        """シーン解析用再帰関数"""

        #オブジェクト名書き込み
        self.write_and_print(file, object.type + " - " + object.name)   
        trans, rot, scale = object.matrix_local.decompose()
        
    def parse_scene_recursive_json(self, data_parent, object, level):
        """シーン解析用再帰関数"""

        #シーンのオブジェクト1個分のjsonオブジェクト作成
        json_object = dict()
        #オブジェクト種類
        json_object["type"] = object.type
        #オブジェクト名
        json_object["name"] = object.name

        #オブジェクトのローカル座標を分解して取得
        trans, rot, scale = object.matrix_local.decompose()
        #回転をオイラー角に変換して、度数表記に変換
        rot = rot.to_euler()
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)
        #トランスフォーム情報をディクショナリに登録
        transform = dict()
        transform["translation"] = (trans.x, trans.y, trans.z)
        transform["rotation"] = (rot.x, rot.y, rot.z)
        transform["scale"] = (scale.x, scale.y, scale.z)
        #まとめてjsonオブジェクトに登録
        json_object["transform"] = transform

        #カスタムプロパティ'filen_name'
        if "file_name" in object:
            json_object["file_name"] = object["file_name"]

        if "spawn_progress" in object:
            json_object["spawn_progress"] = object["spawn_progress"]

        # 敵フラグと設定
        is_enemy = getattr(object, "is_enemy_flag", False) or object.get("is_enemy", False)
        if is_enemy:
            json_object["is_enemy"] = True
            if hasattr(object, "enemy_type"):
                json_object["enemy_type"] = object.enemy_type
            if hasattr(object, "enemy_target") and object.enemy_target:
                json_object["enemy_target_name"] = object.enemy_target.name
                json_object["enemy_target_pos"] = (object.enemy_target.location.x, object.enemy_target.location.y, object.enemy_target.location.z)
            if hasattr(object, "enemy_max_y"):
                json_object["enemy_max_y"] = object.enemy_max_y
            if hasattr(object, "enemy_min_y"):
                json_object["enemy_min_y"] = object.enemy_min_y
            if hasattr(object, "enemy_formation_id"):
                json_object["enemy_formation_id"] = object.enemy_formation_id

        # 破壊フラグ
        if "is_destructible" in object:
            json_object["is_destructible"] = bool(object["is_destructible"])
        else:
            json_object["is_destructible"] = True # デフォルトは破壊可能

        #マテリアルの画像テクスチャ名を取得
        if object.type == 'MESH' and len(object.material_slots) > 0:
            mat = object.material_slots[0].material
            if mat and mat.use_nodes:
                for node in mat.node_tree.nodes:
                    if node.type == 'TEX_IMAGE' and node.image:
                        json_object["texture_path"] = node.image.name
                        break

        #カスタムプロパティ'collider'
        if "collider" in object:
            collider = dict()
            c_type = object.get("collider_type", object.get("collider", "BOX"))
            collider["type"] = c_type
            collider["center"] = object["collider_center"].to_list() if "collider_center" in object else [0,0,0]
            if c_type == 'SPHERE':
                collider["radius"] = object.get("collider_radius", 1.0)
            else:
                collider["size"] = object["collider_size"].to_list() if "collider_size" in object else [2,2,2]
            json_object["collider"] = collider

           # カーブ(レール)情報のエクスポート
        if object.type == 'CURVE':
            json_object["curve_points_debug"] = "Script is updated!"
            curve_data = object.data
            matrix_world = object.matrix_world
            points_list = []
            for spline in curve_data.splines:
                if spline.type == 'BEZIER':
                    for i, point in enumerate(spline.bezier_points):
                        position = matrix_world @ point.co
                        handle_left = matrix_world @ point.handle_left
                        handle_right = matrix_world @ point.handle_right
                        point_data = {
                            "position": {"x": position.x, "y": position.y, "z": position.z},
                            "handle_left": {"x": handle_left.x, "y": handle_left.y, "z": handle_left.z},
                            "handle_right": {"x": handle_right.x, "y": handle_right.y, "z": handle_right.z},
                            "tilt": point.tilt
                        }
                        # オブジェクト自体に持たせた speed_i, event_i を取得する
                        speed_key = f"speed_{i}"
                        event_key = f"event_{i}"
                        if speed_key in object:
                            point_data["speed"] = object[speed_key]
                        if event_key in object:
                            point_data["event"] = object[event_key]
                        points_list.append(point_data)
                elif spline.type in {'NURBS', 'POLY'}:
                    for point in spline.points:
                        position = matrix_world @ mathutils.Vector((point.co.x, point.co.y, point.co.z))
                        point_data = {
                            "position": {"x": position.x, "y": position.y, "z": position.z},
                            "tilt": point.tilt
                        }
                        points_list.append(point_data)
            
            if points_list:
                json_object["curve_points"] = points_list


        #一個分のjsonオブジェクトを親オブジェクトに登録
        data_parent.append(json_object)

        #子ノードがあれば、子ノード分回す
        if len(object.children) > 0:
            #子ノードリストを作成
            json_object["children"] = list()

            #子ノードへ進む
            for child in object.children:
                self.parse_scene_recursive_json(json_object["children"], child, level + 1)






    def export_json(self):
        """シーン情報をJSON形式で出力"""

        #保存する情報をまとめるdict
        json_object_root = dict()

        #ノード名
        json_object_root["name"] = "Scene"
        #オブジェクトリストを作成
        json_object_root["objects"] = list()

        #シーン内の全オブジェクト走査してバック
        for object in bpy.context.scene.objects:
            #親を持たないオブジェクトはスキップ(代わりに親から呼び出すため)
            if(object.parent):
                continue

            #シーン直下のオブジェクトをルートノード(深さ0)とし、再帰関数で走査
            self.parse_scene_recursive_json(json_object_root["objects"], object, 0)

        #オブジェクトをjson文字列にエンコード
        json_text = json.dumps(json_object_root, ensure_ascii=False, cls=json.JSONEncoder, indent=4)
        
        #コンソールに表示
        print(json_text)

        #ファイルをテキスト形式で書き出し用に開く
        #スコープを抜けると自動で閉じる
        with open(self.filepath, 'wt', encoding='utf-8') as file:

            #ファイルにjson文字列を書き込む
            file.write(json_text)

        # --- OBJファイルの自動エクスポート（無効化） ---
        # 毎回OBJが上書きされるのが不便なため、自動エクスポート機能は停止しました。
        # 必要な時だけ手動でエクスポートしてください。
        '''
        import os
        import mathutils

        # JSONの保存先パスを基準に、resources/3dModels のディレクトリパスを計算
        json_dir = os.path.dirname(self.filepath)
        res_idx = json_dir.find("resources")
        if res_idx != -1:
            models_dir = os.path.join(json_dir[:res_idx], "resources", "3dModels")
        else:
            models_dir = json_dir # 見つからない場合のフォールバック
            
        if not os.path.exists(models_dir):
            os.makedirs(models_dir)

        # 現在の選択状態をクリア (コンテキストエラーを避けるため bpy.ops は使わない)
        for obj in bpy.context.scene.objects:
            obj.select_set(False)

        for object in bpy.context.scene.objects:
            if object.type != 'MESH':
                continue

            # ファイル名の決定
            file_name = object.name
            if "file_name" in object and object["file_name"] != "":
                file_name = object["file_name"]
            
            # .objを削除してベース名にする
            base_name = file_name.split('.')[0]
            if not file_name.endswith(".obj"):
                file_name += ".obj"

            # モデル用ディレクトリの作成
            obj_dir = os.path.join(models_dir, base_name)
            if not os.path.exists(obj_dir):
                os.makedirs(obj_dir)

            obj_path = os.path.join(obj_dir, file_name)

            # トランスフォームの一時保存
            saved_location = object.location.copy()
            saved_rotation = object.rotation_euler.copy()
            saved_scale = object.scale.copy()
            
            # オブジェクトを選択状態にしてアクティブに
            object.select_set(True)
            bpy.context.view_layer.objects.active = object

            # 原点へ移動、回転とスケールをリセット (純粋なモデルデータのみ出力するため)
            object.location = mathutils.Vector((0.0, 0.0, 0.0))
            object.rotation_euler = mathutils.Euler((0.0, 0.0, 0.0), 'XYZ')
            object.scale = mathutils.Vector((1.0, 1.0, 1.0))
            
            # 内部の更新を強制
            bpy.context.view_layer.update()

            # エクスポート (Blender 4.0以降は wm.obj_export, それ以前は export_scene.obj)
            try:
                if hasattr(bpy.ops.wm, "obj_export"):
                    bpy.ops.wm.obj_export(filepath=obj_path, export_selected_objects=True, export_triangulated_mesh=True, export_normals=True)
                else:
                    bpy.ops.export_scene.obj(filepath=obj_path, use_selection=True, use_triangles=True, use_normals=True)
            except Exception as e:
                print(f"Failed to export {obj_path}: {e}")

            # トランスフォームを復元
            object.location = saved_location
            object.rotation_euler = saved_rotation
            object.scale = saved_scale
            bpy.context.view_layer.update()

            # 選択解除
            object.select_set(False)
        '''






    def export(self):
        """ファイルに出力"""
        print("シーン情報出力開始... %r" % self.filepath)

        with open(self.filepath, 'wt') as file:

            def write_line(text=""):
                print(text)
                file.write(text + "\n")

            #ファイルに文字列を書き込む
            write_line("SCENE")

            def write_object_tree(obj, indent=0):
                ind = "  " * indent
                write_line(f"{ind}{obj.type} - {obj.name}")

                trans, rot, scale = obj.matrix_local.decompose()
                rot = rot.to_euler()
                rot.x = math.degrees(rot.x)
                rot.y = math.degrees(rot.y)
                rot.z = math.degrees(rot.z)

                write_line(f"{ind}  trans({trans.x:.6f}, {trans.y:.6f}, {trans.z:.6f})")
                write_line(f"{ind}  rot({rot.x:.6f}, {rot.y:.6f}, {rot.z:.6f})")
                write_line(f"{ind}  scale({scale.x:.6f}, {scale.y:.6f}, {scale.z:.6f})")
               # ファイル名があれば出力
                if "file_name" in obj:
                    write_line(f'{ind}  file_name("{obj["file_name"]}")')

                # コライダー情報があれば出力
                if "collider" in obj:
                    write_line(f'{ind}  collider("{obj["collider"]}")')
                    c = obj.get("collider_center")
                    s = obj.get("collider_size")
                    if c is not None:
                        write_line(f'{ind}  collider_center({c[0]:.6f}, {c[1]:.6f}, {c[2]:.6f})')
                    if s is not None:
                        write_line(f'{ind}  collider_size({s[0]:.6f}, {s[1]:.6f}, {s[2]:.6f})')

                # 子オブジェクトを再帰的に書き込む
                for child in obj.children:
                    write_object_tree(child, indent + 1)

            # ルートオブジェクト（親を持たないオブジェクト）からツリーを書き込む
            roots = [o for o in bpy.context.scene.objects if o.parent is None]
            for root in roots:
                write_object_tree(root)
                write_line()

    def execute(self, context):
        print("シーン情報をExportします。")

        #ファイルに出力
        self.export_json()

        print("シーン情報をExportしました。")
        self.report({'INFO'}, "シーン情報をExportしました。")

        return {'FINISHED'}


#オペレータ　OBJ出力
class MYADDON_OT_export_objs(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_export_objs"
    bl_label = "OBJ出力"
    bl_description = "シーン内のメッシュをそれぞれOBJとして一括出力します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        print("OBJの一括Exportを開始します。")

        import os
        import mathutils

        blend_filepath = bpy.data.filepath
        if not blend_filepath:
            self.report({'ERROR'}, "まずBlenderファイルを保存してください（保存先パスを基準に出力します）")
            return {'CANCELLED'}

        json_dir = os.path.dirname(blend_filepath)
        # プロジェクトルート（blender_toolsより上の階層）を探す
        bt_idx = json_dir.find("blender_tools")
        if bt_idx != -1:
            models_dir = os.path.join(json_dir[:bt_idx], "resources", "3dModels")
        else:
            res_idx = json_dir.find("resources")
            if res_idx != -1:
                models_dir = os.path.join(json_dir[:res_idx], "resources", "3dModels")
            else:
                models_dir = os.path.join(json_dir, "resources", "3dModels")
            
        if not os.path.exists(models_dir):
            os.makedirs(models_dir)

        # 現在の選択状態をクリア
        for obj in bpy.context.scene.objects:
            obj.select_set(False)

        export_count = 0
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
                    bpy.ops.wm.obj_export(filepath=obj_path, export_selected_objects=True, export_triangulated_mesh=True, export_normals=True)
                else:
                    bpy.ops.export_scene.obj(filepath=obj_path, use_selection=True, use_triangles=True, use_normals=True)
                export_count += 1
            except Exception as e:
                print(f"Failed to export {obj_path}: {e}")

            object.location = saved_location
            object.rotation_euler = saved_rotation
            object.scale = saved_scale
            bpy.context.view_layer.update()

            object.select_set(False)
            
        print(f"OBJの一括Exportを完了しました。（{export_count}件）")
        self.report({'INFO'}, f"OBJの一括Exportを完了しました。（{export_count}件）")

        return {'FINISHED'}


#トップバーの拡張メニュー
class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"
    bl_description = "拡張メニュー by " + bl_info["author"]

    #サブメニューを描画する関数
    def draw(self, context):
        self.layout.operator("wm.url_open_preset", text="Manual", icon='HELP')
        self.layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text = MYADDON_OT_stretch_vertex.bl_label)
        self.layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text = MYADDON_OT_create_ico_sphere.bl_label)
        self.layout.operator(MYADDON_OT_create_fighter.bl_idname, text = MYADDON_OT_create_fighter.bl_label)
        self.layout.operator(MYADDON_OT_create_asteroid.bl_idname, text = MYADDON_OT_create_asteroid.bl_label)
        self.layout.operator(MYADDON_OT_create_game_camera.bl_idname, text = MYADDON_OT_create_game_camera.bl_label)
        self.layout.operator(MYADDON_OT_export_scene.bl_idname, text = MYADDON_OT_export_scene.bl_label)
        self.layout.operator(MYADDON_OT_export_objs.bl_idname, text = MYADDON_OT_export_objs.bl_label)


    # サブメニューを追加する関数
    def submenu(self, context):
        #IDを指定でサブメニューを追加
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

#登録するクラスリスト
classes  = (
    TOPBAR_MT_my_menu,
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_create_fighter,
    MYADDON_OT_create_asteroid,
    MYADDON_OT_create_game_camera,
    MYADDON_OT_export_scene,
    MYADDON_OT_export_objs,
    MYADDON_OT_add_filename,
    OBJECT_PT_file_name,
    MYADDON_OT_add_collider,
    OBJECT_PT_collider,
    MYADDON_OT_add_destructible,
    OBJECT_PT_destructible,
    OBJECT_PT_enemy_settings,
)

#登録の関数
def register():

    for cls in classes:
        try:
            bpy.utils.unregister_class(cls)
        except:
            pass

    try:
        bpy.types.TOPBAR_MT_editor_menus.remove(
            TOPBAR_MT_my_menu.submenu
        )
    except:
        pass

    for cls in classes:
        bpy.utils.register_class(cls)
    
    # プロパティの登録
    bpy.types.Object.is_enemy_flag = bpy.props.BoolProperty(name="Is Enemy", default=False)
    bpy.types.Object.enemy_type = bpy.props.EnumProperty(
        items=[
            ('RUSHER', "Rusher (突進)", ""),
            ('SHOOTER', "Shooter (弾)", ""),
            ('HOMING', "Homing (ホーミング)", ""),
            ('TURRET', "Turret (固定砲台)", "")
        ],
        name="Enemy Type",
        default='RUSHER'
    )
    bpy.types.Object.enemy_target = bpy.props.PointerProperty(
        type=bpy.types.Object,
        name="Target Position",
        description="移動先となるオブジェクト(Emptyなど)"
    )
    bpy.types.Object.enemy_max_y = bpy.props.FloatProperty(name="Max Y", default=10.0)
    bpy.types.Object.enemy_min_y = bpy.props.FloatProperty(name="Min Y", default=-10.0)
    bpy.types.Object.enemy_formation_id = bpy.props.IntProperty(name="Formation ID", default=-1)

    #メニューに項目を追加
    bpy.types.TOPBAR_MT_editor_menus.append(
        TOPBAR_MT_my_menu.submenu
    )

    #描画関数を3Dビューに追加
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(DrawCollider.draw_collider, (), 'WINDOW', 'POST_VIEW')
    
    print("レベルエディタが有効化されました")


def unregister():

    try:
        bpy.types.TOPBAR_MT_editor_menus.remove(
            TOPBAR_MT_my_menu.submenu
        )
        #描画関数を3Dビューから削除
        bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, 'WINDOW')

        del bpy.types.Object.is_enemy_flag
        del bpy.types.Object.enemy_type
        del bpy.types.Object.enemy_target
        del bpy.types.Object.enemy_max_y
        del bpy.types.Object.enemy_min_y
        del bpy.types.Object.enemy_formation_id
    except:
        pass

    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except:
            pass

    print("レベルエディタが無効化されました")
    
if __name__ == "__main__":
    try:
        unregister()
    except:
        pass

    register()