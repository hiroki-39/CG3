import bpy
import json
import urllib.request
import urllib.error
import math

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
        
        auto_prompt = f"以下のパラメータに従って、全長およそ {props.rail_length}m のカメラレールセクション配列と敵配置データを生成してください。\n"
        auto_prompt += f"・レールの始点の高さ(Z座標): {props.start_z} m からスタートしてください。\n"
        auto_prompt += f"・シチュエーション（演出テーマ）: {sit_names[props.situation]}\n"
        auto_prompt += f"・コース全体の緩急（ペーシング）: {pace_names[props.pacing]}\n"
        auto_prompt += f"・起伏の激しさ (1〜10): レベル {props.intensity}\n"
        auto_prompt += f"・敵の配置頻度: {freq_names[props.event_freq]}\n"
        auto_prompt += f"・【重要】Z座標(高さ)の下限: レールの高さは絶対に {props.min_z} mを下回らないように設計してください。\n"
        
        if props.prompt:
            auto_prompt += f"\n【追加のユーザー要望】\n{props.prompt}\n"

        if not api_key:
            self.report({'ERROR'}, "API Key is missing!")
            return {'CANCELLED'}

        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = f"""
あなたは「スターフォックス」や「パンツァードラグーン」のような名作3Dレールシューティングゲームの、超一流のレベルデザイナーです。
指定されたシチュエーションに基づいて、ステージを構成する「セクション（レールの形状）」の並びと、各セクションに配置する「敵の情報」をJSON形式で出力してください。

【出力JSONフォーマット】
必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。マークダウン(```json)や解説は一切含めないでください。

{{
  "segments": [
    {{
      "type": "STRACTION_TYPE",
      "length": 50.0,
      "speed": 12.0,
      "enemies": [
        {{
          "type": "drone",
          "progress": 0.5,
          "offset_x": -8.0,
          "offset_z": 3.0
        }}
      ]
    }}
  ]
}}

【セクションの種類 ("type")】
- "STRAIGHT": 直進するセクション。
- "CURVE_RIGHT": 右に90度旋回するセクション（lengthは旋回半径として扱われます。最低でも30以上の大きめの値を推奨）。
- "CURVE_LEFT": 左に90度旋回するセクション（lengthは旋回半径として扱われます。最低でも30以上の大きめの値を推奨）。
- "CLIMB": 上昇するセクション。
- "DIVE": 下降するセクション（Z軸下限 {props.min_z}m を下回らないよう注意）。

【敵配置のルール ("enemies")】
- "progress": そのセクション内での出現位置を 0.0（セクション開始点）から 1.0（セクション終了点）の間で指定。
- "offset_x": レール（自機）の進行方向から見た左右のオフセット（マイナスが左、プラスが右）。
- "offset_z": レール（自機）の進行方向から見た上下のオフセット（マイナスが下、プラスが上）。
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
                    generated_text = res_json['candidates'][0]['content']['parts'][0]['text']
                    stage_data = json.loads(generated_text)
                    
                    self.create_stage(stage_data, props.start_z)
                    self.report({'INFO'}, "Stage elements generated successfully!")
                    return {'FINISHED'}
                    
            except urllib.error.HTTPError as e:
                if e.code == 429:
                    self.report({'WARNING'}, "レートリミットに達しました。1分待ってください。")
                    return {'CANCELLED'}
                if e.code == 503 and attempt < max_retries - 1:
                    time.sleep(2 ** (attempt + 1))
                    continue
                self.report({'ERROR'}, f"HTTP Error {e.code}")
                return {'CANCELLED'}
            except Exception as e:
                self.report({'ERROR'}, f"An error occurred: {e}")
                return {'CANCELLED'}

    def create_stage(self, data, start_z):
        segments = data.get("segments", [])
        if not segments:
            return

        # 1. カーブオブジェクトのセットアップ
        curve_data = bpy.data.curves.new('AIRailCurve', type='CURVE')
        curve_data.dimensions = '3D'
        curve_obj = bpy.data.objects.new('AIRail', curve_data)
        bpy.context.scene.collection.objects.link(curve_obj)
        
        spline = curve_data.splines.new('BEZIER')
        
        # 幾何学計算用データ
        current_pos = [0.0, 0.0, start_z]
        current_angle = math.radians(90.0) # 初期方向：+Y方向 (90度)
        
        bezier_points_data = []
        enemy_spawn_list = []

        # 最初の点
        bezier_points_data.append({
            "pos": tuple(current_pos),
            "handle_left": (current_pos[0], current_pos[1] - 5.0, current_pos[2]),
            "handle_right": (current_pos[0], current_pos[1] + 5.0, current_pos[2]),
            "speed": float(segments[0].get("speed", 10.0)),
            "event": "START"
        })

        # 各セクションをパース
        for seg_idx, seg in enumerate(segments):
            seg_type = seg.get("type", "STRAIGHT")
            length = float(seg.get("length", 50.0))
            speed = float(seg.get("speed", 12.0))
            enemies = seg.get("enemies", [])
            
            seg_start_pos = list(current_pos)
            seg_start_angle = current_angle

            # --- 直線、上昇、下降の処理 ---
            if seg_type in ["STRAIGHT", "CLIMB", "DIVE"]:
                z_offset = length * 0.3 if seg_type == "CLIMB" else (-length * 0.3 if seg_type == "DIVE" else 0.0)
                dx = length * math.cos(current_angle)
                dy = length * math.sin(current_angle)
                
                current_pos[0] += dx
                current_pos[1] += dy
                current_pos[2] += z_offset
                
                # ハンドルの長さ（滑らかにつなぐためのベジェの重み）
                h_len = length * 0.2
                hl = (current_pos[0] - h_len * math.cos(current_angle), current_pos[1] - h_len * math.sin(current_angle), current_pos[2])
                hr = (current_pos[0] + h_len * math.cos(current_angle), current_pos[1] + h_len * math.sin(current_angle), current_pos[2])
                
                bezier_points_data.append({
                    "pos": tuple(current_pos), "handle_left": hl, "handle_right": hr, "speed": speed, "event": seg_type
                })
                
                # 敵の配置（直線用リニア補間）
                for enemy in enemies:
                    prog = max(0.0, min(1.0, float(enemy.get("progress", 0.5))))
                    ox = float(enemy.get("offset_x", 0.0))
                    oz = float(enemy.get("offset_z", 0.0))
                    
                    ex_c = seg_start_pos[0] + dx * prog
                    ey_c = seg_start_pos[1] + dy * prog
                    ez_c = seg_start_pos[2] + z_offset * prog
                    
                    right_angle = current_angle - math.radians(90.0)
                    ex = ex_c + ox * math.cos(right_angle)
                    ey = ey_c + ox * math.sin(right_angle)
                    ez = ez_c + oz
                    enemy_spawn_list.append({"type": enemy.get("type", "drone"), "loc": (ex, ey, ez), "seg": seg_idx})

            # --- 右旋回・左旋回の処理（90度カーブを細かく割って超滑らかにする） ---
            elif seg_type in ["CURVE_RIGHT", "CURVE_LEFT"]:
                is_right = (seg_type == "CURVE_RIGHT")
                radius = max(10.0, length) # 最低半径を保証
                
                # 回転の中心点を計算
                center_offset_angle = current_angle - math.radians(90.0) if is_right else current_angle + math.radians(90.0)
                cx = current_pos[0] + radius * math.cos(center_offset_angle)
                cy = current_pos[1] + radius * math.sin(center_offset_angle)
                
                # 90度を何分割するか（6分割 = 15度ずつ打つことで超滑らかに）
                steps = 6
                angle_step = math.radians(90.0) / steps
                
                start_arc_angle = current_angle + math.radians(90.0) if is_right else current_angle - math.radians(90.0)
                
                for step in range(1, steps + 1):
                    # 現在のステップの角度
                    if is_right:
                        arc_angle = start_arc_angle - angle_step * step
                        current_angle = arc_angle - math.radians(90.0)
                    else:
                        arc_angle = start_arc_angle + angle_step * step
                        current_angle = arc_angle + math.radians(90.0)
                        
                    # 新しい点の座標
                    current_pos[0] = cx + radius * math.cos(arc_angle)
                    current_pos[1] = cy + radius * math.sin(arc_angle)
                    
                    # カーブの滑らかさを保つ接線ハンドルの計算
                    # ベジェ曲線で円弧を近似するための最適なハンドルの長さ
                    h_len = radius * (4.0 / 3.0) * math.tan(angle_step / 4.0)
                    
                    hl = (current_pos[0] - h_len * math.cos(current_angle), current_pos[1] - h_len * math.sin(current_angle), current_pos[2])
                    hr = (current_pos[0] + h_len * math.cos(current_angle), current_pos[1] + h_len * math.sin(current_angle), current_pos[2])
                    
                    bezier_points_data.append({
                        "pos": tuple(current_pos), "handle_left": hl, "handle_right": hr, "speed": speed, "event": f"{seg_type}_{step}"
                    })
                
                # 敵の配置（円弧に沿った補間計算）
                for enemy in enemies:
                    prog = max(0.0, min(1.0, float(enemy.get("progress", 0.5))))
                    ox = float(enemy.get("offset_x", 0.0))
                    oz = float(enemy.get("offset_z", 0.0))
                    
                    # 進行度に応じた円弧上の角度
                    e_step_angle = math.radians(90.0) * prog
                    if is_right:
                        e_arc_angle = start_arc_angle - e_step_angle
                        e_current_angle = e_arc_angle - math.radians(90.0)
                    else:
                        e_arc_angle = start_arc_angle + e_step_angle
                        e_current_angle = e_arc_angle + math.radians(90.0)
                        
                    ex_c = cx + radius * math.cos(e_arc_angle)
                    ey_c = cy + radius * math.sin(e_arc_angle)
                    
                    right_angle = e_current_angle - math.radians(90.0)
                    ex = ex_c + ox * math.cos(right_angle)
                    ey = ey_c + ox * math.sin(right_angle)
                    ez = current_pos[2] + oz
                    enemy_spawn_list.append({"type": enemy.get("type", "drone"), "loc": (ex, ey, ez), "seg": seg_idx})

        # 3. Blenderのベジェ曲線オブジェクトに一気に反映
        spline.bezier_points.add(len(bezier_points_data) - 1)
        for idx, pt in enumerate(bezier_points_data):
            bp = spline.bezier_points[idx]
            bp.co = pt["pos"]
            bp.handle_left = pt["handle_left"]
            bp.handle_right = pt["handle_right"]
            # ハンドルの種類をFREEにして、計算通りの綺麗な向きに固定する
            bp.handle_left_type = 'FREE'
            bp.handle_right_type = 'FREE'
            
            curve_obj[f"speed_{idx}"] = pt["speed"]
            curve_obj[f"event_{idx}"] = pt["event"]

        # 4. 敵オブジェクトを配置
        for e_idx, enemy in enumerate(enemy_spawn_list):
            bpy.ops.mesh.primitive_ico_sphere_add(radius=2.0, location=enemy["loc"])
            enemy_obj = bpy.context.active_object
            enemy_obj.name = f"Enemy_{enemy['type']}_Seg{enemy['seg']}_{e_idx}"
            enemy_obj["enemy_type"] = enemy["type"]
            enemy_obj["parent_rail"] = curve_obj.name

        # レールを選択状態にする
        bpy.context.view_layer.objects.active = curve_obj
        curve_obj.select_set(True)


class AIRAIL_PT_Panel(bpy.types.Panel):
    bl_label = "AI自動レール生成"
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
        layout.operator(AIRAIL_OT_Generate.bl_idname, text="AIでステージ・敵を自動生成", icon='OUTLINER_OB_CURVE')

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