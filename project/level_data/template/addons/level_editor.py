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
        # 頂点データ
        vertices ={"pos": []}
        # インデックスデータ
        indices = []

        #各頂点の、オブジェクト中心からのオフセット
        offsets = [
                    [ -0.5, -0.5, -0.5], #左下前
                    [ +0.5, -0.5, -0.5], #右下前
                    [ -0.5, +0.5, -0.5], #左上前
                    [ +0.5, +0.5, -0.5], #右上前
                    [ -0.5, -0.5, +0.5], #左下奥
                    [ +0.5, -0.5, +0.5], #右下奥
                    [ -0.5, +0.5, +0.5], #左上奥
                    [ +0.5, +0.5, +0.5], #右上奥
                   
                ]
        
        #立方体のX,Y,Z方向のサイズ
        size = [2.0, 2.0, 2.0]

        #現在シーンのオブジェクトリストを走査
        for object in bpy.context.scene.objects:
            #コライダープロパティがなければ描画スキップ
            if not "collider" in object:
                continue

            #中心点、サイズの変数を宣言
            center = mathutils.Vector((0.0, 0.0, 0.0))
            size = mathutils.Vector((2.0, 2.0, 2.0))

            #プロパティから値を取得
            center[0] = object["collider_center"][0]
            center[1] = object["collider_center"][1]
            center[2] = object["collider_center"][2]
            size[0] = object["collider_size"][0]
            size[1] = object["collider_size"][1]
            size[2] = object["collider_size"][2]

            #追加前の頂点数
            start = len(vertices["pos"])

            #Boxの8頂点分回す
            for offset in offsets:
                #オブジェクトの中心座標をコピー

                pos = copy.copy(center)
                #中心点を基準に各頂点座標をコピー
                pos[0] += offset[0] * size[0]
                pos[1] += offset[1] * size[1]
                pos[2] += offset[2] * size[2]
                #ローカル座標からワールド座標に変換
                pos = object.matrix_world @ pos

                #頂点データリストに座標を追加
                vertices["pos"].append(pos)

                #前面を構成する返の頂点インデックス
                indices.append([start + 0, start + 1])
                indices.append([start + 2, start + 3])
                indices.append([start + 0, start + 2])
                indices.append([start + 1, start + 3])

                #奥面を構成する辺の頂点インデックス
                indices.append([start + 4, start + 5])
                indices.append([start + 6, start + 7])
                indices.append([start + 4, start + 6])
                indices.append([start + 5, start + 7])

                #手前と奥をつなぐ辺の頂点インデックス
                indices.append([start + 0, start + 4])
                indices.append([start + 1, start + 5])
                indices.append([start + 2, start + 6])
                indices.append([start + 3, start + 7])


        #　ビルドインのシェーダーを取得
        shader = gpu.shader.from_builtin('UNIFORM_COLOR')

        # バッチを作成
        batch = gpu_extras.batch.batch_for_shader(shader, 'LINES', vertices, indices = indices)

        #シェーダーのパラメータ設定
        color = [0.5, 1.0, 1.0, 1.0]
        shader.bind()
        shader.uniform_float("color", color)
        #描画
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
        #[collider]プロパティを追加
        context.object["collider"] = "BOX"
        context.object["collider_center"] = mathutils.Vector((0.0, 0.0, 0.0))
        context.object["collider_size"] = mathutils.Vector((2.0, 2.0, 2.0))

        return {'FINISHED'}

#パネル ファイル名
class OBJECT_PT_file_name(bpy.types.Panel):
    """オブジェクトのファイルネームパネル"""
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "FileName"
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
    bl_label = "Collider"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        #パネルに項目を追加
        if "collider" in context.object:
            #既にプロパティがあれば、プロパティを表示
            self.layout.prop(context.object, '["collider"]', text= self.bl_label)
            self.layout.prop(context.object, '["collider_center"]', text= "Collider Center")
            self.layout.prop(context.object, '["collider_size"]', text= "Collider Size")
        else:
            #プロパティがなければ、プロパティ追加のオペレータを表示
            self.layout.operator(MYADDON_OT_add_collider.bl_idname)

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

        #カスタムプロパティ'collider'
        if "collider" in object:
            collider = dict()
            collider["type"] = object["collider"]
            collider["center"] = object["collider_center"].to_list()
            collider["size"] = object["collider_size"].to_list()
            json_object["collider"] = collider
            

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
        self.layout.operator(MYADDON_OT_export_scene.bl_idname, text = MYADDON_OT_export_scene.bl_label)


    # サブメニューを追加する関数
    def submenu(self, context):
        #IDを指定でサブメニューを追加
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

#登録するクラスリスト
classes  = (
    TOPBAR_MT_my_menu,
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_export_scene,
    MYADDON_OT_add_filename,
    OBJECT_PT_file_name,
    MYADDON_OT_add_collider,
    OBJECT_PT_collider,
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