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
    total_length: bpy.props.IntProperty(
        name="目安の全長 (m)",
        description="コース全体の長さ",
        default=3000,
        min=100,
        max=10000
    )
    prompt: bpy.props.StringProperty(
        name="プロンプト",
        description="作成したいステージの地形環境を記述してください",
        default="市街地を抜けて、なだらかな丘陵を越え、最後は険しい渓谷に入る",
    )

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
def shape_terrain_profile(obj, preset, length):
    """
    カテゴリごとに地形の断面（プロファイル）を成形する
    """
    import math
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
    fade_margin = 150.0
    half_length = length / 2.0
    
    # 蛇行（うねり）のパラメータ
    wave_amplitude = 60.0
    wave_length = 300.0
    
    for v in mesh.vertices:
        y = v.co.y
        center_x = math.sin(y / wave_length) * wave_amplitude
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
            
            # 共通のベースシェイプ（高さ40mのなだらかなU字谷）を計算
            if dist_x <= half_path:
                path_ratio = dist_x / half_path
                blend = 0.15 + 0.85 * (path_ratio ** 2)
                base_shape_z = base_noise * 40.0 * blend
            else:
                out_dist = dist_x - half_path
                edge_dist = half_max - half_path
                ratio = out_dist / edge_dist
                if ratio > 1.0: ratio = 1.0
                curve = ratio * ratio * (3.0 - 2.0 * ratio)
                base_shape_z = 40.0 * curve + (base_noise * 10.0)
            
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
            
            # 2. マッピングノードの自動配置（巨大な地形でもボヤけないよう20回リピート設定）
            mapping = mat.node_tree.nodes.new('ShaderNodeMapping')
            mapping.location = (-500, 300)
            mapping.inputs['Scale'].default_value = (20.0, 20.0, 20.0)
            
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

    current_y = 0.0

    for seg_idx, seg in enumerate(segments):
        raw_category = seg.get("category", "PLAINS")
        category = str(raw_category).upper().replace("-", "_")
        
        # モデル全体を2倍スケールにするため、長さ(奥行き)も2倍にする
        length = float(seg.get("length", 500.0)) * 2.0
        
        # プリセットからパラメータを取得（存在しないカテゴリはPLAINSになる）
        preset = TERRAIN_PRESETS.get(category, TERRAIN_PRESETS["PLAINS"])
        
        width = preset["width"]
        height = preset["height"]          # U字の絶壁の高さ
        noise_height = preset["noise_height"] # 岩肌のゴツゴツ具合(振幅)
        noise_type = preset["noise_type"]
        subd_x = preset["subd_x"]
        noise_size = preset["noise_size"]
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
        shape_terrain_profile(obj, preset, length)
        
        # --- 追加処理: マテリアルの自動割り当て ---
        assign_terrain_material(obj, category)
        
        # 生成されたオブジェクトを AI_Terrains コレクションに移動
        # (Landscape Add するとデフォルトで現在のコレクションに入るため、移動させる)
        for coll in obj.users_collection:
            coll.objects.unlink(obj)
        terrain_collection.objects.link(obj)

        # 頂点を中心(0,0,0)からずらして配置する
        # A.N.T.Landscapeは原点を中心にして生成するため、Y方向に length / 2 ずらす
        # さらにこれまでの current_y を足すことで、地形が前後に連結される
        obj.location.y = current_y + (length / 2.0)
        
        # 次のセグメントのスタート位置を更新
        current_y += length

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
        
        # 遠景用のマテリアル（MOUNTAINSを流用）を自動割り当て
        assign_terrain_material(bg_obj, "MOUNTAINS")


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
        
        auto_prompt = f"コースの全長は {props.total_length}m です。ユーザーのプロンプトに基づいて、全長を満たすように複数の地形セグメントに分割し、JSONで出力してください。\n\n"
        auto_prompt += f"【プロンプト】\n{props.prompt}\n"

        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = f"""
あなたは3Dゲームの環境デザイナーです。
指定された全長と情景プロンプトを分析し、最適な地形カテゴリを組み合わせて、セグメントの配列として出力してください。

【出力JSONフォーマット】
必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。
複数のセグメント（区間）を出力し、length（長さ）の合計が指定された全長と大体同じになるようにしてください。

{{
  "segments": [
    {{
      "length": 1000.0,
      "category": "CITY"
    }},
    {{
      "length": 1500.0,
      "category": "HILLS"
    }},
    {{
      "length": 500.0,
      "category": "CANYON"
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
        
        box = layout.box()
        box.label(text="システム設定:")
        box.prop(props, "total_length")
        
        layout.separator()
        layout.label(text="AIプロンプト:")
        layout.prop(props, "prompt", text="")
        
        layout.separator()
        layout.operator(AITERRAIN_OT_Generate.bl_idname, text="地形モデルを自動生成", icon='MESH_GRID')
        
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
