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
        "width": 200, "height": 10, "noise_height": 3, "noise_type": 'multi_fractal', 
        "subd_x": 128, "noise_size": 3.0
    },
    "CITY": {
        "width": 200, "height": 15, "noise_height": 2, "noise_type": 'strata_hterrain', 
        "subd_x": 128, "noise_size": 2.0
    },
    "CANYON": {
        "width": 200, "height": 150, "noise_height": 50, "noise_type": 'ridged_multi_fractal', 
        "subd_x": 128, "noise_size": 4.0
    },
    "MOUNTAINS": {
        "width": 200, "height": 150, "noise_height": 60, "noise_type": 'hetero_terrain', 
        "subd_x": 128, "noise_size": 5.0
    },
    "LUNAR": {
        "width": 200, "height": 50, "noise_height": 20, "noise_type": 'planet_noise', 
        "subd_x": 128, "noise_size": 3.0
    },
    "HILLS": {
        "width": 200, "height": 40, "noise_height": 15, "noise_type": 'multi_fractal', 
        "subd_x": 128, "noise_size": 4.0
    },
    "PLAINS": {
        "width": 200, "height": 5, "noise_height": 2, "noise_type": 'multi_fractal', 
        "subd_x": 128, "noise_size": 3.0
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
def shape_terrain_profile(obj, path_width=80.0, max_width=200.0, target_edge_height=50.0):
    """
    地形の中央（レールが通る部分）を平らにし、
    両端に行くにつれて指定の高さ(target_edge_height)まで強制的に隆起させる。
    A.N.T.Landscapeのノイズ起伏も加味する。
    """
    mesh = obj.data
    half_path = path_width / 2.0
    half_max = max_width / 2.0
    
    for v in mesh.vertices:
        x = abs(v.co.x)
        original_z = v.co.z
        
        if x <= half_path:
            # 中央のレール道は高さを0にする
            v.co.z = 0.0
        else:
            # 平坦な道から、メッシュの端(half_max)にかけての割合 (0.0 ~ 1.0)
            ratio = (x - half_path) / (half_max - half_path)
            if ratio > 1.0: ratio = 1.0
            
            # なだらかに立ち上がり、端で急峻になるカーブ (ratio^2 などで調整可能)
            curve = ratio * ratio * (3.0 - 2.0 * ratio)
            
            # 基本の崖の高さ(U字型) ＋ 元のノイズの起伏(端にいくほど強く出る)
            v.co.z = (target_edge_height * curve) + (original_z * curve)
            
    # メッシュを更新
    mesh.update()

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
        length = float(seg.get("length", 500.0))
        
        # プリセットからパラメータを取得（存在しないカテゴリはPLAINSになる）
        preset = TERRAIN_PRESETS.get(category, TERRAIN_PRESETS["PLAINS"])
        
        width = preset["width"]
        height = preset["height"]          # U字の絶壁の高さ
        noise_height = preset["noise_height"] # 岩肌のゴツゴツ具合(振幅)
        noise_type = preset["noise_type"]
        subd_x = preset["subd_x"]
        noise_size = preset["noise_size"]
        
        # 奥行き(Y)の分割数は、長さに比例させる（5mに1ポリゴン、最大256）
        subd_y = min(256, max(64, int(length / 5.0)))
        
        # ランダムシードで毎回違う地形にする
        seed = random.randint(0, 100000)

        # 確実に新しく作られたオブジェクトを取得するための準備
        objects_before = set(bpy.context.scene.objects)

        # A.N.T.Landscapeでメッシュを生成
        # ゴツゴツ感を出すため、heightにはnoise_heightを適用する
        bpy.ops.mesh.landscape_add(
            mesh_size_x=width, 
            mesh_size_y=length,
            subdivision_x=subd_x, 
            subdivision_y=subd_y,
            noise_type=noise_type,
            noise_size=noise_size,
            noise_depth=8, # より細かい岩肌のディテールを出すため 6 -> 8 に増やす
            height=noise_height, 
            noise_offset_x=random.uniform(-100, 100),
            noise_offset_y=random.uniform(-100, 100),
            random_seed=seed,
            refresh=True
        )
        
        # 追加されたオブジェクトを取得
        objects_after = set(bpy.context.scene.objects)
        new_objects = objects_after - objects_before
        
        if not new_objects:
            raise Exception(f"地形({category})の生成に失敗しました。A.N.T.Landscapeが反応していません。")
            
        obj = new_objects.pop()
        obj.name = f"AITerrain_{seg_idx:02d}_{category}"
        
        # --- 追加処理: 地形のU字型整形 ---
        # ユーザーの提案通り、両端の高さを強制的にプリセットの高さ(height)まで引き上げます
        # canyonなら150m、cityなら15mなど、確実に高低差を出します。
        shape_terrain_profile(obj, path_width=80.0, max_width=width, target_edge_height=height)
        
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
