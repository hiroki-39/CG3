import bpy
import json
import urllib.request
import urllib.error
import threading
import random

# =================================================================
# 1. プリセット定義
# AIが選択したカテゴリに応じて、A.N.T.Landscape用のパラメータを設定
# =================================================================
TERRAIN_PRESETS = {
    "OPEN_SEA": {
        "width": 2000, "height": 10, "noise_height": 10, "noise_type": 'multi_fractal', 
        "subd_x": 128, "noise_size": 30.0, "profile_type": "SEA", "path_width": 200.0
    },
    "CITY": {
        "width": 1000, "height": 15, "noise_height": 5, "noise_type": 'strata_hterrain', 
        "subd_x": 128, "noise_size": 10.0, "profile_type": "FLAT", "path_width": 200.0
    },
    "CANYON": {
        "width": 1000, "height": 100, "noise_height": 20, "noise_type": 'multi_fractal', 
        "subd_x": 64, "noise_size": 25.0, "profile_type": "CLIFF", "path_width": 200.0
    },
    "MOUNTAINS": {
        "width": 2000, "height": 200, "noise_height": 50, "noise_type": 'hetero_terrain', 
        "subd_x": 128, "noise_size": 40.0, "profile_type": "CLIFF", "path_width": 200.0
    },
    "LUNAR": {
        "width": 2000, "height": 50, "noise_height": 20, "noise_type": 'planet_noise', 
        "subd_x": 128, "noise_size": 20.0, "profile_type": "GENTLE", "path_width": 200.0
    },
    "HILLS": {
        "width": 2000, "height": 80, "noise_height": 60, "noise_type": 'hetero_terrain', 
        "subd_x": 128, "noise_size": 400.0, "noise_depth": 3, "profile_type": "OPEN", "path_width": 200.0
    },
    "PLAINS": {
        "width": 2000, "height": 5, "noise_height": 10, "noise_type": 'multi_fractal', 
        "subd_x": 128, "noise_size": 30.0, "profile_type": "FLAT", "path_width": 200.0
    }
}


# =================================================================
# 2. UIプロパティ定義
# =================================================================
class AITerrainGeneratorProperties(bpy.types.PropertyGroup):
    api_key: bpy.props.StringProperty(
        name="API Key",
        description="Enter your Google Gemini API Key",
        default="",
        subtype='PASSWORD'
    )

    prompt: bpy.props.StringProperty(
        name="プロンプト",
        description="作成したいステージの地形環境を記述してください",
        default="市街地を抜けて、なだらかな丘陵を越え、最後は険しい渓谷に入る",
    )
    len_open_sea: bpy.props.IntProperty(name="海 (OPEN_SEA)", default=1000, min=100, max=10000)
    len_city: bpy.props.IntProperty(name="市街地 (CITY)", default=1000, min=100, max=10000)
    len_canyon: bpy.props.IntProperty(name="渓谷 (CANYON)", default=1000, min=100, max=10000)
    len_mountains: bpy.props.IntProperty(name="山脈 (MOUNTAINS)", default=2000, min=100, max=10000)
    len_lunar: bpy.props.IntProperty(name="月面 (LUNAR)", default=1000, min=100, max=10000)
    len_hills: bpy.props.IntProperty(name="丘陵 (HILLS)", default=1500, min=100, max=10000)
    len_plains: bpy.props.IntProperty(name="平原 (PLAINS)", default=1000, min=100, max=10000)

# 非同期通信用の状態管理
class AITerrainGenState:
    is_running = False
    terrain_data = None
    error_msg = None


# =================================================================
# 3. AI通信処理
# =================================================================
def fetch_gemini_terrain_data(url, payload):
    try:
        data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(req) as response:
            res_body = response.read().decode('utf-8')
            res_json = json.loads(res_body)
            generated_text = res_json['candidates'][0]['content']['parts'][0]['text']
            
            # マークダウン(```json)対策
            import re
            match = re.search(r'\{.*\}', generated_text, re.DOTALL)
            if match:
                generated_text = match.group(0)
                
            terrain_data = json.loads(generated_text)
            AITerrainGenState.terrain_data = terrain_data
    except Exception as e:
        import traceback
        traceback.print_exc()
        AITerrainGenState.error_msg = str(e)
    finally:
        AITerrainGenState.is_running = False

def show_message_box(message="", title="エラー", icon='ERROR'):
    def draw(self, context):
        self.layout.label(text=message)
    try:
        bpy.context.window_manager.popup_menu(draw, title=title, icon=icon)
    except:
        pass

def check_ai_terrain_gen_thread():
    if AITerrainGenState.is_running:
        return 0.5 # 0.5秒後に再チェック
        
    if AITerrainGenState.error_msg:
        err_str = AITerrainGenState.error_msg
        print("AI Terrain Gen Error:", err_str)
        
        # 画面にポップアップ表示
        if "503" in err_str:
            show_message_box("AIサーバーが混雑しています（503エラー）。時間を置いて再度お試しください。", title="通信エラー")
        elif "429" in err_str:
            show_message_box("AIの利用回数制限に達しました（429エラー）。1〜2分ほど待ってから再度お試しください。", title="通信制限エラー")
        else:
            show_message_box(f"AI通信エラー: {err_str}", title="通信エラー")
            
        # 詳細をログに書き出す
        with open(r"c:\Users\k024g\Lesson\2025A\CG2\CG2\project\blender_tools\error.log", "w") as f:
            f.write("AI Terrain Gen Error:\n" + err_str)
            
    elif AITerrainGenState.terrain_data:
        try:
            create_terrains(AITerrainGenState.terrain_data)
            print("Terrain generated successfully!")
        except Exception as e:
            import traceback
            traceback.print_exc()
            err_str = str(e)
            print("Create Terrain Error:", err_str)
            show_message_box(f"地形生成エラー: {err_str} (詳細は error.log を確認)", title="処理エラー")
            with open(r"c:\Users\k024g\Lesson\2025A\CG2\CG2\project\blender_tools\error.log", "w") as f:
                f.write("Create Terrain Error:\n" + traceback.format_exc())
            
    return None # タイマー終了


# =================================================================
# 4. 地形生成処理 (A.N.T.Landscape使用)
# =================================================================
def shape_terrain_profile(obj, preset, length, current_y, global_offset):
    """
    カテゴリごとに地形の断面（プロファイル）を成形する
    """
    import math
    import mathutils
    mesh = obj.data
    
    # ---------------------------
    # 地形の幅や道の幅をプリセットから取得
    # ---------------------------
    path_width = preset.get("path_width", 200.0)
    max_width = preset["width"]
    target_edge_height = preset["height"]
    noise_height = preset["noise_height"]
    profile_type = preset.get("profile_type", "FLAT")
    
    half_path = path_width / 2.0
    half_max = max_width / 2.0
    
    # 前後のアセットと繋ぐためのフェード（グラデーション）幅
    fade_margin = 250.0
    half_length = length / 2.0
    
    # 蛇行（うねり）のパラメータ
    wave_amplitude = 60.0
    wave_length = 300.0
    
    for v in mesh.vertices:
        y = v.co.y
        # ワールドY座標（コース全体の開始位置からの距離）を計算
        y_world = current_y + y + half_length
        
        # ワールド座標とグローバルオフセットを使って蛇行（道）の中心を計算
        # これにより、毎回違う方向に曲がる道が生成される
        center_x = math.sin((y_world + global_offset) / wave_length) * wave_amplitude
        dist_x = abs(v.co.x - center_x)
        base_noise = v.co.z
        
        if dist_x <= half_path:
            # 川（道）の中
            path_ratio = dist_x / half_path
            # 道も完全に平坦ではなく、ルートの道しるべになる程度(15%)の高低差を持たせる
            if profile_type == "SEA":
                v.co.z = (base_noise * noise_height * 2.0) * (path_ratio ** 2)
            else:
                blend = 0.15 + 0.85 * (path_ratio ** 2)
                v.co.z = base_noise * noise_height * blend
        else:
            out_dist = dist_x - half_path
            edge_dist = half_max - half_path
            
            if profile_type == "CLIFF":
                # キャニオンや山脈：切り立った絶壁
                cliff_width = 30.0 + (math.sin(y / 50.0) * 15.0)
                if out_dist <= cliff_width:
                    ratio = out_dist / cliff_width
                    current_height = target_edge_height * ratio
                    v.co.z = current_height + (base_noise * noise_height)
                else:
                    v.co.z = target_edge_height + (base_noise * noise_height)
                    
            elif profile_type == "GENTLE":
                # 丘陵など：なだらかな立ち上がり
                ratio = out_dist / edge_dist
                if ratio > 1.0: ratio = 1.0
                # Smoothstepカーブ
                curve = ratio * ratio * (3.0 - 2.0 * ratio)
                current_height = target_edge_height * curve
                v.co.z = current_height + (base_noise * noise_height)
                
            elif profile_type == "SEA":
                # 海：平原よりも波のようなうねりを大きくする
                ratio = out_dist / edge_dist
                if ratio > 1.0: ratio = 1.0
                current_height = target_edge_height * ratio
                # 海面の波を表現するためノイズを倍増
                v.co.z = current_height + (base_noise * noise_height * 2.0)
                
            elif profile_type == "OPEN":
                # スターフォックスのコーネリア後半のような開けた平原・丘陵
                # 全体が平坦になりすぎないよう、外側に向けてなだらかにベースを盛り上げる
                ratio = out_dist / edge_dist
                if ratio > 1.0: ratio = 1.0
                
                # Smoothstepカーブでなだらかに盛り上げる
                curve = ratio * ratio * (3.0 - 2.0 * ratio)
                current_height = target_edge_height * curve
                
                # なだらかな起伏（ノイズ）を全体に乗せる
                v.co.z = current_height + (base_noise * noise_height)
                
            else: # FLAT
                # 平原など：ほぼ平坦（わずかな起伏のみ）
                ratio = out_dist / edge_dist
                if ratio > 1.0: ratio = 1.0
                current_height = target_edge_height * ratio
                v.co.z = current_height + (base_noise * noise_height)
        
        # ---------------------------
        # ★重要: 前後のアセットと完全に繋げるためのフェード処理
        # Y軸の端（始点と終点）に近づくにつれて、地形の形状を「共通のベースシェイプ（高さ40mのなだらかな丘）」にフェードさせる
        # これにより、急な凹み（谷底）ができず、全てのアセットが自然な高さで繋がります
        # ---------------------------
        dist_from_start = y - (-half_length)
        dist_from_end = half_length - y
        min_dist_to_edge = min(dist_from_start, dist_from_end)
        
        if min_dist_to_edge < fade_margin:
            # 端に近づくほど 0 になる係数 (Smoothstepで滑らかに)
            fade_ratio = min_dist_to_edge / fade_margin
            fade_curve = fade_ratio * fade_ratio * (3.0 - 2.0 * fade_ratio)
            
            # 境界部分のYワールド座標（これを使うことで、前後のアセットで完全に同じノイズが生成される）
            coord = mathutils.Vector((v.co.x / 100.0, (y_world + global_offset) / 100.0, 0.0))
            shared_noise = mathutils.noise.noise(coord)
            
            # 共通のベースシェイプ（高さ40mのなだらかなU字谷 + 共通のノイズ）を計算
            if dist_x <= half_path:
                path_ratio = dist_x / half_path
                blend = 0.15 + 0.85 * (path_ratio ** 2)
                base_shape_z = shared_noise * 30.0 * blend
            else:
                out_dist = dist_x - half_path
                edge_dist = half_max - half_path
                ratio = out_dist / edge_dist
                if ratio > 1.0: ratio = 1.0
                curve = ratio * ratio * (3.0 - 2.0 * ratio)
                base_shape_z = 40.0 * curve + (shared_noise * 30.0)
            
            # 現在の地形(v.co.z)と共通ベースシェイプ(base_shape_z)をブレンド
            v.co.z = (v.co.z * fade_curve) + (base_shape_z * (1.0 - fade_curve))

    # メッシュを更新
    mesh.update()

def assign_terrain_material(obj, category):
    import os
    mat_name = f"Mat_{category}"
    
    # 既に同じ名前のマテリアルがあればそれを使い回す
    if mat_name in bpy.data.materials:
        mat = bpy.data.materials[mat_name]
    else:
        # 新規マテリアル作成
        mat = bpy.data.materials.new(name=mat_name)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get("Principled BSDF")
        
        if bsdf:
            # カテゴリに応じた仮のベースカラーを設定（画像がない場合用）
            base_color = (0.5, 0.5, 0.5, 1.0)
            if category in ["HILLS", "PLAINS"]: base_color = (0.2, 0.5, 0.2, 1.0)
            elif category in ["CANYON", "MOUNTAINS"]: base_color = (0.4, 0.25, 0.15, 1.0)
            elif category == "OPEN_SEA": base_color = (0.1, 0.3, 0.8, 1.0)
            
            # 1. 画像テクスチャノードの自動配置
            tex_image = mat.node_tree.nodes.new('ShaderNodeTexImage')
            tex_image.location = (-300, 300)
            mat.node_tree.links.new(tex_image.outputs['Color'], bsdf.inputs['Base Color'])
            
            # 2. マッピングノードの自動配置（UV自体を20倍にしているため、ここでは等倍に戻す）
            mapping = mat.node_tree.nodes.new('ShaderNodeMapping')
            mapping.location = (-500, 300)
            mapping.inputs['Scale'].default_value = (1.0, 1.0, 1.0)
            
            # 3. テクスチャ座標ノードの自動配置
            tex_coord = mat.node_tree.nodes.new('ShaderNodeTexCoord')
            tex_coord.location = (-700, 300)
            
            # ノードをリンク
            mat.node_tree.links.new(tex_coord.outputs['UV'], mapping.inputs['Vector'])
            mat.node_tree.links.new(mapping.outputs['Vector'], tex_image.inputs['Vector'])
            
            # 4. (おまけ) 近くに画像ファイルがあれば自動ロードを試みる
            blend_dir = os.path.dirname(bpy.data.filepath) if bpy.data.filepath else ""
            if blend_dir:
                tex_dir = os.path.join(blend_dir, "textures")
                if os.path.exists(tex_dir):
                    for ext in [".png", ".jpg"]:
                        img_path = os.path.join(tex_dir, f"{category.lower()}{ext}")
                        if os.path.exists(img_path):
                            img = bpy.data.images.load(filepath=img_path)
                            tex_image.image = img
                            break
            
            if not tex_image.image:
                bsdf.inputs['Base Color'].default_value = base_color

    # オブジェクトにマテリアルを割り当て
    if obj.data.materials:
        obj.data.materials[0] = mat
    else:
        obj.data.materials.append(mat)

def create_rail_for_terrain(total_length, global_offset):
    import math
    import json
    import os
    
    curve_data = bpy.data.curves.new('AITerrainRailCurve', type='CURVE')
    curve_data.dimensions = '3D'
    curve_obj = bpy.data.objects.new('AITerrainRail', curve_data)
    
    collection_name = "AI_Terrains"
    if collection_name in bpy.data.collections:
        bpy.data.collections[collection_name].objects.link(curve_obj)
    else:
        bpy.context.scene.collection.objects.link(curve_obj)
        
    spline = curve_data.splines.new('BEZIER')
    
    # shape_terrain_profile と同じうねりパラメータを使用
    wave_amplitude = 60.0
    wave_length = 300.0
    interval = 50.0
    
    points_count = int(total_length / interval) + 1
    spline.bezier_points.add(points_count - 1)
    
    curve_points_json = []
    
    for i in range(points_count):
        y = i * interval
        if y > total_length:
            y = total_length
            
        # 地形と同じ計算式で道の中央Xを求める
        center_x = math.sin((y + global_offset) / wave_length) * wave_amplitude
        z = 20.0 # 地面から少し浮かせた位置
        
        bp = spline.bezier_points[i]
        bp.co = (center_x, y, z)
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
        
        # JSON用のデータ (LevelLoader.cpp の想定フォーマット)
        curve_points_json.append({
            "position": {"x": center_x, "y": z, "z": y}, # C++側で x, z(up), y(forward) に変換して読まれるためそのまま出力
            "handle_left": {"x": center_x, "y": z, "z": y - 5.0},
            "handle_right": {"x": center_x, "y": z, "z": y + 5.0},
            "tilt": 0.0,
            "speed": 12.0,
            "event": "STRAIGHT"
        })
        
    # JSONのエクスポート
    export_dir = r"c:\Users\k024g\Lesson\2025A\CG2\CG2\project\resources\json\maps"
    if not os.path.exists(export_dir):
        os.makedirs(export_dir, exist_ok=True)
        
    json_path = os.path.join(export_dir, "ai_level_data.json")
    export_data = {
        "objects": [
            {
                "curve_points": curve_points_json
            }
        ]
    }
    
    try:
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(export_data, f, indent=4)
        print(f"レールデータをJSONにエクスポートしました: {json_path}")
    except Exception as e:
        print(f"JSONエクスポートエラー: {e}")

def create_terrains(data):
    segments = data.get("segments", [])
    if not segments:
        return

    # A.N.T.Landscapeアドオンが有効か確認
    if not hasattr(bpy.ops.mesh, "landscape_add"):
        raise Exception("A.N.T.Landscape アドオンが有効になっていません。プリファレンスから有効化してください。")

    # 地形をまとめるためのコレクション（フォルダ）を作成
    collection_name = "AI_Terrains"
    if collection_name not in bpy.data.collections:
        terrain_collection = bpy.data.collections.new(collection_name)
        bpy.context.scene.collection.children.link(terrain_collection)
    else:
        terrain_collection = bpy.data.collections[collection_name]
        # 前回の生成モデルとメッシュを全削除してクリーンアップ
        for obj in list(terrain_collection.objects):
            mesh = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            if mesh:
                bpy.data.meshes.remove(mesh, do_unlink=True)

    current_y = 0.0
    props = bpy.context.scene.ai_terrain_props
    
    # 毎回全く違うコース（道のうねりや崖の形）になるように、全体で共通のランダムオフセットを生成
    global_offset = random.uniform(0.0, 100000.0)

    for seg_idx, seg in enumerate(segments):
        raw_category = seg.get("category", "PLAINS")
        category = str(raw_category).upper().replace("-", "_")
        
        # ユーザーがUIパネルで設定した各カテゴリごとのベース長さを取得する
        # AIがJSONで指定した length は無視し、確実に設定通りの長さにします
        prop_name = f"len_{category.lower()}"
        if hasattr(props, prop_name):
            base_length = getattr(props, prop_name)
        else:
            base_length = 1000.0
            
        length = float(base_length)
        
        # プリセットからベースパラメータを取得（存在しないカテゴリはPLAINSになる）
        preset = TERRAIN_PRESETS.get(category, TERRAIN_PRESETS["PLAINS"])
        
        # AIが出力したJSONからパラメータを取得。なければプリセットの固定値を使用する
        width = float(seg.get("width", preset["width"]))
        height = float(seg.get("height", preset["height"]))          # U字の絶壁の高さ
        noise_height = float(seg.get("noise_height", preset["noise_height"])) # 岩肌のゴツゴツ具合(振幅)
        noise_size = float(seg.get("noise_size", preset["noise_size"]))
        path_width = float(seg.get("path_width", preset.get("path_width", 200.0)))
        
        # 形状成形用に関数へ渡すカスタムプリセットを作成
        preset_custom = preset.copy()
        preset_custom["width"] = width
        preset_custom["height"] = height
        preset_custom["noise_height"] = noise_height
        preset_custom["noise_size"] = noise_size
        preset_custom["path_width"] = path_width
        
        noise_type = preset["noise_type"]
        subd_x = preset["subd_x"]
        noise_depth = preset.get("noise_depth", 6) # ディテールレベル(細かいボコボコ感)
        
        # 奥行き(Y)の分割数は、長さに比例させる（約25mに1ポリゴン、最大128）
        # ポリゴンを大きめにして、紙くしゃくしゃではなく「大きな岩塊」にする
        subd_y = min(128, max(32, int(length / 25.0)))
        
        # ランダムシードで毎回違う地形にする
        seed = random.randint(0, 100000)

        # 確実に新しく作られたオブジェクトを取得するための準備
        objects_before = set(bpy.context.scene.objects)

        # A.N.T.Landscapeでメッシュを生成
        # タイマーからの実行時 (poll failed) を回避するため、3Dビューのコンテキストを完全にオーバーライド
        win = bpy.context.window
        scr = win.screen if win else None
        area = next((a for a in scr.areas if a.type == 'VIEW_3D'), None) if scr else None
        region = next((r for r in area.regions if r.type == 'WINDOW'), None) if area else None

        def run_landscape_add():
            bpy.ops.mesh.landscape_add(
                mesh_size_x=width, 
                mesh_size_y=length,
                subdivision_x=subd_x, 
                subdivision_y=subd_y,
                noise_type=noise_type,
                noise_size=noise_size,
                noise_depth=noise_depth,
                height=1.0, 
                noise_offset_x=random.uniform(-100, 100),
                noise_offset_y=random.uniform(-100, 100),
                random_seed=seed,
                refresh=True
            )

        if win and scr and area and region:
            with bpy.context.temp_override(window=win, screen=scr, area=area, region=region):
                # エディットモード等になっていると失敗するため、オブジェクトモードに戻す
                if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
                    try:
                        bpy.ops.object.mode_set(mode='OBJECT')
                    except:
                        pass
                run_landscape_add()
        else:
            # フォールバック
            if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
                try:
                    bpy.ops.object.mode_set(mode='OBJECT')
                except:
                    pass
            run_landscape_add()
        
        # 追加されたオブジェクトを取得
        objects_after = set(bpy.context.scene.objects)
        new_objects = objects_after - objects_before
        
        if not new_objects:
            raise Exception(f"地形({category})の生成に失敗しました。A.N.T.Landscapeが反応していません。")
            
        obj = new_objects.pop()
        obj.name = f"AITerrain_{seg_idx:02d}_{category}"
        
        # --- 追加処理: 地形のU字型整形とゴツゴツ増幅 ---
        # プリセットに応じて崖の形状を変える（キャニオンは絶壁、丘はなだらかに等）
        shape_terrain_profile(obj, preset_custom, length, current_y, global_offset)
        
        # --- 変更: マテリアルの自動生成は一時停止したまま、UVの書き込みを復活 ---
        # Blenderのマッピングノードはゲームエンジン(OBJ/FBX)にエクスポートされないため、
        # メッシュのUVデータ自体に「10mに1回リピートする」設定を直接書き込みます。
        if not obj.data.uv_layers:
            obj.data.uv_layers.new(name="UVMap")
        uv_layer = obj.data.uv_layers.active.data
        for poly in obj.data.polygons:
            for loop_index in poly.loop_indices:
                loop = obj.data.loops[loop_index]
                vert = obj.data.vertices[loop.vertex_index]
                # 実際のX, Y座標を元に、10m四方で画像が1枚貼られるようにUVを計算
                uv_layer[loop_index].uv = (vert.co.x / 10.0, vert.co.y / 10.0)
        
        # assign_terrain_material(obj, category)

        
        # 生成されたオブジェクトを AI_Terrains コレクションに移動
        # (Landscape Add するとデフォルトで現在のコレクションに入るため、移動させる)
        for coll in obj.users_collection:
            coll.objects.unlink(obj)
        terrain_collection.objects.link(obj)

        # 頂点を中心(0,0,0)からずらして配置する
        # A.N.T.Landscapeは原点を中心にして生成するため、Y方向に length / 2 ずらす
        # さらにこれまでの current_y を足すことで、地形が前後に連結される
        obj.location.y = current_y + (length / 2.0)
        

        
        current_y += length

    # 全ての地形セグメントを作り終えた後、総延長に対してレールを1本敷き、JSON出力する
    create_rail_for_terrain(current_y, global_offset)

    # ---------------------------
    # 追加処理: 遠景モデル（背景の巨大山脈）の自動生成
    # メイン地形の外側を覆うように、非常に巨大で粗い山脈を生成します
    # ---------------------------
    total_length = current_y
    bg_width = 8000.0
    bg_length = total_length + 4000.0 # 前後にも余裕を持たせる
    
    objects_before = set(bpy.context.scene.objects)
    
    def run_bg_landscape_add():
        bpy.ops.mesh.landscape_add(
            mesh_size_x=bg_width, 
            mesh_size_y=bg_length,
            subdivision_x=128, 
            subdivision_y=128, # 背景なのでポリゴンは粗めでOK
            noise_type='hetero_terrain',
            noise_size=1000.0, # 非常に巨大なうねり
            noise_depth=4,
            height=1.0, 
            noise_offset_x=random.uniform(-100, 100),
            noise_offset_y=random.uniform(-100, 100),
            random_seed=random.randint(0, 100000),
            refresh=True
        )

    if win and scr and area and region:
        with bpy.context.temp_override(window=win, screen=scr, area=area, region=region):
            if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
                try: bpy.ops.object.mode_set(mode='OBJECT')
                except: pass
            run_bg_landscape_add()
    else:
        if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
            try: bpy.ops.object.mode_set(mode='OBJECT')
            except: pass
        run_bg_landscape_add()
        
    objects_after = set(bpy.context.scene.objects)
    new_objects = objects_after - objects_before
    
    if new_objects:
        bg_obj = new_objects.pop()
        bg_obj.name = "AITerrain_Background_Mountains"
        
        for coll in bg_obj.users_collection:
            coll.objects.unlink(bg_obj)
        terrain_collection.objects.link(bg_obj)
        
        # Z原点を中心から始点へ
        bg_obj.location.y = total_length / 2.0
        
        # 背景地形のプロファイル成形
        # 中央（メイン地形がある場所）は邪魔にならないように低く沈め、外側を巨大な山脈にする
        mesh = bg_obj.data
        for v in mesh.vertices:
            dist_x = abs(v.co.x)
            base_noise = v.co.z
            
            if dist_x < 1500.0:
                # メイン地形(幅2000)の下に隠れるように -100m に沈める
                v.co.z = -100.0
            else:
                # 1500m ~ 3500m かけて徐々に高さ400mまで盛り上がる
                ratio = (dist_x - 1500.0) / 2000.0
                if ratio > 1.0: ratio = 1.0
                curve = ratio * ratio * (3.0 - 2.0 * ratio)
                
                v.co.z = -100.0 + (curve * 400.0) + (base_noise * 150.0)
        
        mesh.update()
        
        # 遠景モデルも同様にUVを直接書き込む（遠景なので100m四方に1枚リピートとする）
        if not bg_obj.data.uv_layers:
            bg_obj.data.uv_layers.new(name="UVMap")
        bg_uv_layer = bg_obj.data.uv_layers.active.data
        for poly in bg_obj.data.polygons:
            for loop_index in poly.loop_indices:
                loop = bg_obj.data.loops[loop_index]
                vert = bg_obj.data.vertices[loop.vertex_index]
                bg_uv_layer[loop_index].uv = (vert.co.x / 100.0, vert.co.y / 100.0)
        
        # assign_terrain_material(bg_obj, "MOUNTAINS")


# =================================================================
# 5. オペレーターとパネル定義
# =================================================================
class AITERRAIN_OT_Generate(bpy.types.Operator):
    bl_idname = "object.ai_terrain_generate"
    bl_label = "Generate AI Terrain"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        if AITerrainGenState.is_running:
            self.report({'WARNING'}, "現在AIが生成中です... お待ちください。")
            return {'CANCELLED'}
            
        props = context.scene.ai_terrain_props
        api_key = props.api_key
        
        if not api_key:
            self.report({'ERROR'}, "API Key is missing!")
            return {'CANCELLED'}
            
        AITerrainGenState.is_running = True
        AITerrainGenState.terrain_data = None
        AITerrainGenState.error_msg = None
        
        auto_prompt = f"ユーザーのプロンプトに基づいて、複数の地形セグメントのリストをJSONで出力してください。\n\n"
        auto_prompt += f"【プロンプト】\n{props.prompt}\n"

        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = f"""
あなたは3Dゲームの環境デザイナーです。
情景プロンプトを分析し、最適な地形カテゴリを組み合わせて、セグメントの配列として出力してください。

【基準となるパラメータ目安】
地形を生成する際のベースとなる数値です。この数値を基準にして、情景に合わせて±20%程度ランダムに数値をアレンジして出力してください。
- OPEN_SEA: width:2000, height:10, noise_height:10, noise_size:30.0, path_width:200.0
- CITY: width:1000, height:15, noise_height:5, noise_size:10.0, path_width:200.0
- CANYON: width:1000, height:100, noise_height:20, noise_size:25.0, path_width:200.0
- MOUNTAINS: width:2000, height:200, noise_height:50, noise_size:40.0, path_width:200.0
- LUNAR: width:2000, height:50, noise_height:20, noise_size:20.0, path_width:200.0
- HILLS: width:2000, height:80, noise_height:60, noise_size:400.0, path_width:200.0
- PLAINS: width:2000, height:5, noise_height:10, noise_size:30.0, path_width:200.0

【出力JSONフォーマット】
必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。
複数のセグメント（区間）を配列で出力します。lengthは無視されますがスキーマの都合上適当な数値を入れてください。

{{
  "segments": [
    {{
      "length": 1000.0,
      "category": "CITY",
      "width": 1000.0,
      "height": 14.5,
      "noise_height": 5.2,
      "noise_size": 11.0,
      "path_width": 180.0
    }},
    {{
      "length": 1500.0,
      "category": "HILLS",
      "width": 2100.0,
      "height": 75.0,
      "noise_height": 65.0,
      "noise_size": 380.0,
      "path_width": 220.0
    }}
  ]
}}

【選択可能なカテゴリ】
- "OPEN_SEA": 海や平原など、大きく開けた空間
- "CITY": 市街地や人工的なブロック状の地形
- "CANYON": 切り立った鋭い尾根を持つ渓谷
- "MOUNTAINS": 複雑で起伏の激しい山脈
- "LUNAR": クレーターが点在する月面や荒野
- "HILLS": なだらかな丘陵や緑地
- "PLAINS": 平らな平原
"""
        payload = {
            "contents": [{
                "parts": [{"text": system_prompt + "\n\nUser Request:\n" + auto_prompt}]
            }],
            "generationConfig": {
                "temperature": 1.2,
                "response_mime_type": "application/json"
            }
        }
        
        self.report({'INFO'}, "AIに地形の構成をリクエストしました！生成中...")
        
        # スレッド起動（フリーズ回避）
        thread = threading.Thread(target=fetch_gemini_terrain_data, args=(url, payload))
        thread.start()
        
        # タイマー登録
        if not bpy.app.timers.is_registered(check_ai_terrain_gen_thread):
            bpy.app.timers.register(check_ai_terrain_gen_thread)
        
        return {'FINISHED'}


class AITERRAIN_PT_Panel(bpy.types.Panel):
    bl_label = "AI自動地形生成 (A.N.T.Landscape版)"
    bl_idname = "AITERRAIN_PT_Panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'AI Terrain'

    def draw(self, context):
        layout = self.layout
        props = context.scene.ai_terrain_props

        layout.prop(props, "api_key")
        
        layout.separator()
        layout.label(text="AIプロンプト:")
        layout.prop(props, "prompt", text="")
        
        # 各カテゴリの長さを手動設定できるパネルボックス
        len_box = layout.box()
        len_box.label(text="地形ごとの長さ設定 (m):")
        len_box.prop(props, "len_open_sea")
        len_box.prop(props, "len_city")
        len_box.prop(props, "len_canyon")
        len_box.prop(props, "len_mountains")
        len_box.prop(props, "len_lunar")
        len_box.prop(props, "len_hills")
        len_box.prop(props, "len_plains")
        
        layout.separator()
        layout.operator(AITERRAIN_OT_Generate.bl_idname, text="地形とレールを自動生成＆エクスポート", icon='MESH_GRID')
        
        if AITerrainGenState.is_running:
            layout.label(text="🔄 AIが地形を構成中...", icon='TIME')

classes = (
    AITerrainGeneratorProperties,
    AITERRAIN_OT_Generate,
    AITERRAIN_PT_Panel,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.ai_terrain_props = bpy.props.PointerProperty(type=AITerrainGeneratorProperties)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.ai_terrain_props
    
    if bpy.app.timers.is_registered(check_ai_terrain_gen_thread):
        bpy.app.timers.unregister(check_ai_terrain_gen_thread)

if __name__ == "__main__":
    register()
