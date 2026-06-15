import bpy
import json
import urllib.request
import urllib.error

class AIRailGeneratorProperties(bpy.types.PropertyGroup):
    api_key: bpy.props.StringProperty(
        name="API Key",
        description="Enter your Google Gemini API Key",
        default="",
        subtype='PASSWORD'
    )
    rail_length: bpy.props.IntProperty(
        name="全長 (m)",
        description="レールの長さを指定します",
        default=500,
        min=10,
        max=5000
    )
    situation: bpy.props.EnumProperty(
        name="シチュエーション",
        description="コースの演出テーマを選択します",
        items=[
            ('CANYON', "渓谷フライト", "狭い谷間を縫うように激しく切り返すコース"),
            ('BATTLESHIP', "巨大戦艦急襲", "巨大な対象の周囲を旋回・急降下するコース"),
            ('ASTEROID', "アステロイドベルト", "障害物を避ける小刻みで予測不能なコース"),
            ('SKYDIVE', "大気圏突入", "高高度から一気に急降下し地表を駆けるコース"),
            ('NORMAL', "標準コース", "ベーシックなコース")
        ],
        default='CANYON'
    )
    pacing: bpy.props.EnumProperty(
        name="コースの緩急",
        description="レール全体のテンポ（Pacing）",
        items=[
            ('LATE_CLIMAX', "後半で激化", "最初は静かで、進むにつれて激しくなる"),
            ('ROLLERCOASTER', "ジェットコースター", "激しい起伏と穏やかな直線を交互に繰り返す"),
            ('CONSTANT_HIGH', "常にクライマックス", "最初から最後まで最高難易度のアクロバット飛行"),
            ('NORMAL', "一定ペース", "常に一定の難易度で進む")
        ],
        default='ROLLERCOASTER'
    )
    intensity: bpy.props.IntProperty(
        name="起伏の激しさ",
        description="カーブや上下移動の激しさ",
        default=5,
        min=1,
        max=10
    )
    event_freq: bpy.props.EnumProperty(
        name="イベント頻度",
        description="敵などのイベントの発生頻度",
        items=[
            ('NONE', "なし", "イベントを配置しない"),
            ('LOW', "少ない", "たまに配置する"),
            ('NORMAL', "普通", "標準的な頻度で配置する"),
            ('HIGH', "多い", "頻繁にイベントを配置する")
        ],
        default='NORMAL'
    )
    min_z: bpy.props.FloatProperty(
        name="Z座標の下限 (m)",
        description="レールがこれより下に潜らないようにします（地面の高さ）",
        default=0.0,
    )
    start_z: bpy.props.FloatProperty(
        name="始点の高さ (Z座標)",
        description="レールがスタートする地点の高さを設定します",
        default=0.0,
    )
    prompt: bpy.props.StringProperty(
        name="追加の自由記述",
        description="特別な要望があれば記入してください（空欄でも可）",
        default="",
    )

class AIRAIL_OT_Generate(bpy.types.Operator):
    bl_idname = "object.ai_rail_generate"
    bl_label = "Generate AI Rail"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        props = context.scene.ai_rail_props
        api_key = props.api_key
        
        # UIのパラメータからプロンプトを自動生成
        sit_names = {
            'CANYON': '渓谷フライト（狭い谷間を縫うような激しい切り返しと起伏）',
            'BATTLESHIP': '巨大戦艦急襲（巨大な対象の周囲を大きく旋回・急上昇・急降下する映画的カメラワーク）',
            'ASTEROID': 'アステロイドベルト（障害物を避ける小刻みで予測不能な軌道）',
            'SKYDIVE': '大気圏突入（超高高度から一気に急降下し、地表スレスレを高速で滑空）',
            'NORMAL': '標準的なコース'
        }
        pace_names = {
            'LATE_CLIMAX': '後半で激化（序盤は静か、終盤で急激に激しくなる）',
            'ROLLERCOASTER': 'ジェットコースター（激しい起伏と穏やかな直線の繰り返し）',
            'CONSTANT_HIGH': '常にクライマックス（終始アクロバティック）',
            'NORMAL': '一定のペース'
        }
        freq_names = {'NONE': '全くなし', 'LOW': '少ない', 'NORMAL': '普通', 'HIGH': '非常に多い'}
        
        auto_prompt = f"以下のパラメータに従って、全長 {props.rail_length}m のカメラレールを生成してください。\n"
        auto_prompt += f"・レールの始点の高さ(Z座標): {props.start_z} m からスタートしてください。\n"
        auto_prompt += f"・シチュエーション（演出テーマ）: {sit_names[props.situation]}\n"
        auto_prompt += f"・コース全体の緩急（ペーシング）: {pace_names[props.pacing]}\n"
        auto_prompt += f"・起伏の激しさ (1〜10): レベル {props.intensity}\n"
        auto_prompt += f"・道中のイベント発生頻度: {freq_names[props.event_freq]}\n"
        auto_prompt += f"・【重要】Z座標(高さ)の下限: 絶対に {props.min_z} mを下回らない（マイナスに行かない等）ようにしてください。\n"
        
        if props.prompt:
            auto_prompt += f"\n【追加のユーザー要望】\n{props.prompt}\n"
            
        print("Generated Auto Prompt:\n", auto_prompt)

        if not api_key:
            self.report({'ERROR'}, "API Key is missing!")
            return {'CANCELLED'}

        # Gemini APIリクエストの準備
        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = """
あなたは「スターフォックス」や「パンツァードラグーン」のような名作3Dレールシューティングゲームの、超一流のレベルデザイナーです。
指定されたシチュエーションに基づいて、ベジェ曲線の制御点（位置、左ハンドル、右ハンドル）、速度、イベント情報、そして「敵や障害物の配置情報」をJSON形式で出力してください。

【プロのレベルデザイン要求】
1. 単なる数学的な曲線ではなく、プレイヤーの手に汗握るような「ダイナミックな演出」と「カメラワークの緩急」を意識してください。
2. 急旋回、急降下、見せ場となる長い直線など、シチュエーションに応じたドラマチックな軌道を計算して座標を生成してください。
3. ベジェハンドルの長さを工夫してください。ハンドルを長くして滑らかな大旋回を作ったり、ハンドルを短くして鋭角な回避行動を表現してください。
4. 【追加】指定されたシチュエーションやイベント頻度に合わせて、コース上に敵（enemies）や障害物（obstacles）を配置してください。

必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。マークダウン(```json)や余分な解説を含めないでください。
{
  "points": [
    {
      "position": {"x": 0.0, "y": 0.0, "z": 0.0},
      "handle_left": {"x": 0.0, "y": -5.0, "z": 0.0},
      "handle_right": {"x": 0.0, "y": 5.0, "z": 0.0},
      "speed": 10.0,
      "event": "none"
    }
  ],
  "enemies": [
    {
      "type": "Fighter",
      "spawn_point_index": 5,
      "offset_x": 10.0,
      "offset_y": 0.0,
      "offset_z": 5.0
    }
  ],
  "obstacles": [
    {
      "type": "Asteroid",
      "spawn_point_index": 12,
      "offset_x": -5.0,
      "offset_y": 0.0,
      "offset_z": -2.0
    }
  ]
}
【重要ルール】
・レールの始点(1つ目の制御点)のX座標とY座標は、必ず (0.0, 0.0) にしてください。Z座標についてはユーザーリクエストの指定に従ってください。
・レールが伸びる基本の進行方向は、必ず「+Y方向」(Y軸の正の方向) としてください。
・長さや曲がり具合は、+Y方向に進みながらX軸(左右)やZ軸(上下)を変化させて表現してください。
・ハンドルの座標は絶対座標(グローバル座標)で指定してください。進行方向が+Yなので、基本的なハンドルはY軸方向に伸ばす(+Yや-Y)形になります。
・【配置のルール】敵や障害物は、レールの絶対座標ではなく「レール上の何番目のポイント(spawn_point_index)を基準にするか」と、「そこからの相対位置(offset_x, offset_y, offset_z)」で指定してください。これにより地形にめり込むのを防ぎます。
"""
        payload = {
            "contents": [{
                "parts": [{"text": system_prompt + "\n\nUser Request:\n" + auto_prompt}]
            }],
            "generationConfig": {
                "response_mime_type": "application/json"
            }
        }
        
        data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
        
        import time
        max_retries = 3
        
        for attempt in range(max_retries):
            try:
                with urllib.request.urlopen(req) as response:
                    res_body = response.read().decode('utf-8')
                    res_json = json.loads(res_body)
                    
                    # Geminiのレスポンスからテキストを抽出
                    generated_text = res_json['candidates'][0]['content']['parts'][0]['text']
                    
                    # JSONとしてパース
                    rail_data = json.loads(generated_text)
                    
                    self.create_rail(rail_data)
                    self.report({'INFO'}, "Rail generated successfully!")
                    return {'FINISHED'}
                    
            except urllib.error.HTTPError as e:
                error_msg = e.read().decode('utf-8')
                # 503エラー(混雑)の場合は自動リトライ
                if e.code == 503 and attempt < max_retries - 1:
                    wait_time = 2 ** (attempt + 1) # 2秒, 4秒と待機時間を増やす
                    print(f"Server is busy (503). Retrying in {wait_time} seconds... (Attempt {attempt+1}/{max_retries})")
                    time.sleep(wait_time)
                    continue
                
                self.report({'ERROR'}, f"HTTP Error {e.code}")
                print(f"--- API Error Details ---\n{error_msg}\n-----------------------")
                return {'CANCELLED'}
            except urllib.error.URLError as e:
                self.report({'ERROR'}, f"API Request failed: {e.reason}")
                return {'CANCELLED'}
            except json.JSONDecodeError as e:
                self.report({'ERROR'}, f"Failed to parse AI response as JSON: {e}")
                if 'generated_text' in locals():
                    print(generated_text)
                return {'CANCELLED'}
            except Exception as e:
                self.report({'ERROR'}, f"An error occurred: {e}")
                return {'CANCELLED'}

    def create_rail(self, data):
        points = data.get("points", [])
        if not points:
            return

        # カーブデータとオブジェクトの作成
        curve_data = bpy.data.curves.new('AIRailCurve', type='CURVE')
        curve_data.dimensions = '3D'
        curve_obj = bpy.data.objects.new('AIRail', curve_data)
        bpy.context.scene.collection.objects.link(curve_obj)
        
        # スプラインの作成
        spline = curve_data.splines.new('BEZIER')
        spline.bezier_points.add(len(points) - 1)
        
        for i, pt_data in enumerate(points):
            bp = spline.bezier_points[i]
            
            pos = pt_data.get("position", {"x":0, "y":0, "z":0})
            hl = pt_data.get("handle_left", {"x":-1, "y":0, "z":0})
            hr = pt_data.get("handle_right", {"x":1, "y":0, "z":0})
            
            bp.co = (pos["x"], pos["y"], pos["z"])
            bp.handle_left = (hl["x"], hl["y"], hl["z"])
            bp.handle_right = (hr["x"], hr["y"], hr["z"])
            
            # カスタムプロパティの設定 (オブジェクトに保存)
            # 制御点(BezierSplinePoint)には直接カスタムプロパティを付けられないため、
            # オブジェクト自体に speed_0, event_0 のような名前で保存します。
            curve_obj[f"speed_{i}"] = float(pt_data.get("speed", 1.0))
            curve_obj[f"event_{i}"] = str(pt_data.get("event", "none"))

        # オブジェクトを選択状態にする
        bpy.context.view_layer.objects.active = curve_obj
        curve_obj.select_set(True)

        # ---- 追加：敵・障害物の自動配置（プレースホルダー） ----
        enemies = data.get("enemies", [])
        for i, e_data in enumerate(enemies):
            idx = e_data.get("spawn_point_index", 0)
            if idx >= len(points): idx = len(points) - 1
            
            base_pos = points[idx].get("position", {"x":0, "y":0, "z":0})
            offset_x = e_data.get("offset_x", 0.0)
            offset_y = e_data.get("offset_y", 0.0)
            offset_z = e_data.get("offset_z", 0.0)
            
            e_type = e_data.get("type", "Enemy")
            obj_name = f"Enemy_{e_type}_{i}"
            
            # ダミーのEmptyオブジェクト（四角枠）を配置
            empty_obj = bpy.data.objects.new(obj_name, None)
            empty_obj.empty_display_type = 'CUBE'
            empty_obj.empty_display_size = 2.0
            empty_obj.location = (base_pos["x"] + offset_x, base_pos["y"] + offset_y, base_pos["z"] + offset_z)
            
            # ゲーム側にエクスポートするためのカスタムプロパティ
            empty_obj["file_name"] = e_type
            empty_obj["is_enemy"] = True
            bpy.context.scene.collection.objects.link(empty_obj)

        obstacles = data.get("obstacles", [])
        for i, o_data in enumerate(obstacles):
            idx = o_data.get("spawn_point_index", 0)
            if idx >= len(points): idx = len(points) - 1
            
            base_pos = points[idx].get("position", {"x":0, "y":0, "z":0})
            offset_x = o_data.get("offset_x", 0.0)
            offset_y = o_data.get("offset_y", 0.0)
            offset_z = o_data.get("offset_z", 0.0)
            
            o_type = o_data.get("type", "Obstacle")
            obj_name = f"Obstacle_{o_type}_{i}"
            
            # ダミーのEmptyオブジェクト（球枠）を配置
            empty_obj = bpy.data.objects.new(obj_name, None)
            empty_obj.empty_display_type = 'SPHERE'
            empty_obj.empty_display_size = 3.0
            empty_obj.location = (base_pos["x"] + offset_x, base_pos["y"] + offset_y, base_pos["z"] + offset_z)
            
            empty_obj["file_name"] = o_type
            empty_obj["is_obstacle"] = True
            bpy.context.scene.collection.objects.link(empty_obj)

class AIRAIL_PT_Panel(bpy.types.Panel):
    bl_label = "AI Rail Generator"
    bl_idname = "AIRAIL_PT_Panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'AI Rail'

    def draw(self, context):
        layout = self.layout
        props = context.scene.ai_rail_props

        layout.prop(props, "api_key")
        
        box = layout.box()
        box.label(text="ダイナミック生成パラメータ設定:")
        box.prop(props, "rail_length")
        box.prop(props, "situation")
        box.prop(props, "pacing")
        box.prop(props, "intensity", slider=True)
        box.prop(props, "event_freq")
        box.prop(props, "start_z")
        box.prop(props, "min_z")
        
        layout.prop(props, "prompt", text="追加要望(任意)")
        layout.operator(AIRAIL_OT_Generate.bl_idname, text="AIでレールを自動生成", icon='OUTLINER_OB_CURVE')

classes = (
    AIRailGeneratorProperties,
    AIRAIL_OT_Generate,
    AIRAIL_PT_Panel,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.ai_rail_props = bpy.props.PointerProperty(type=AIRailGeneratorProperties)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.ai_rail_props

if __name__ == "__main__":
    register()
