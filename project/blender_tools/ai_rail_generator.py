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
        name="目安の全長 (m)",
        description="レールの長さの目安（AIへの指示用）",
        default=1000,
        min=10,
        max=5000
    )
    min_z: bpy.props.FloatProperty(
        name="Z座標の下限 (m)",
        description="レールがこれより下に潜らないようにします（地面の高さなど）",
        default=0.0,
    )
    start_z: bpy.props.FloatProperty(
        name="始点の高さ (Z座標)",
        description="レールがスタートする地点の高さを設定します",
        default=0.0,
    )
    prompt: bpy.props.StringProperty(
        name="プロンプト",
        description="作成したいステージのイメージを自由に記述してください（例: 障害物を避ける激しいコース）",
        default="アステロイド帯を縫うような激しいコース",
    )

class AIRAIL_OT_Generate(bpy.types.Operator):
    bl_idname = "object.ai_rail_generate"
    bl_label = "Generate AI Rail"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        props = context.scene.ai_rail_props
        api_key = props.api_key
        
        auto_prompt = f"以下の条件に従って、全長およそ {props.rail_length}m のカメラレールセクション配列と敵配置データを生成してください。\n"
        auto_prompt += f"・レールの始点の高さ(Z座標): {props.start_z} m からスタートしてください。\n"
        auto_prompt += f"・【重要】Z座標(高さ)の下限: レールの高さは絶対に {props.min_z} mを下回らないように設計してください。\n"
        
        if props.prompt:
            auto_prompt += f"\n【ユーザーの希望するコースイメージ（この内容に沿って起伏や敵の配置を考えてください）】\n{props.prompt}\n"
        else:
            auto_prompt += "\n【ユーザーの希望するコースイメージ】\nおまかせでカッコいいコースを作ってください。\n"

        if not api_key:
            self.report({'ERROR'}, "API Key is missing!")
            return {'CANCELLED'}

        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = f"""
あなたは「スターフォックス」や「パンツァードラグーン」のような名作3Dレールシューティングゲームの、超一流のレベルデザイナーです。
指定されたイメージに基づいて、ステージを構成する「セクション（レールの形状）」の並びと、各セクションに配置する「敵や障害物の情報」をJSON形式で出力してください。

【出力JSONフォーマット】
必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。マークダウン(```json)や解説は一切含めないでください。

{{
  "segments": [
    {{
      "type": "STRAIGHT",
      "length": 50.0,
      "speed": 12.0,
      "enemies": [
        {{
          "type": "Fighter",
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
- "type": 敵の種類。指定可能なのは "Fighter"（戦闘機）、"Asteroid"（障害物）、"Enemy"（汎用敵）などです。
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

        # レール全体の長さを計算（spawn_progressの算出用）
        total_length = 0.0
        for seg in segments:
            seg_type = seg.get("type", "STRAIGHT")
            length = float(seg.get("length", 50.0))
            if seg_type in ["CURVE_RIGHT", "CURVE_LEFT"]:
                radius = max(10.0, length)
                total_length += radius * math.pi / 2.0 # 円弧の長さ
            else:
                total_length += length
        
        # 最初の点
        bezier_points_data.append({
            "pos": tuple(current_pos),
            "handle_left": (current_pos[0], current_pos[1] - 5.0, current_pos[2]),
            "handle_right": (current_pos[0], current_pos[1] + 5.0, current_pos[2]),
            "speed": float(segments[0].get("speed", 10.0)),
            "event": "START"
        })

        current_cumulative_len = 0.0

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
                    
                    spawn_prog = (current_cumulative_len + length * prog) / total_length if total_length > 0 else 0.0
                    enemy_spawn_list.append({"type": enemy.get("type", "Fighter"), "loc": (ex, ey, ez), "seg": seg_idx, "spawn_progress": spawn_prog})
                    
                current_cumulative_len += length

            # --- 右旋回・左旋回の処理（90度カーブを細かく割って超滑らかにする） ---
            elif seg_type in ["CURVE_RIGHT", "CURVE_LEFT"]:
                is_right = (seg_type == "CURVE_RIGHT")
                radius = max(10.0, length) # 最低半径を保証
                arc_length = radius * math.pi / 2.0
                
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
                    
                    spawn_prog = (current_cumulative_len + arc_length * prog) / total_length if total_length > 0 else 0.0
                    enemy_spawn_list.append({"type": enemy.get("type", "Fighter"), "loc": (ex, ey, ez), "seg": seg_idx, "spawn_progress": spawn_prog})
                
                current_cumulative_len += arc_length

        # 3. Blenderのベジェ曲線オブジェクトに一気に反映
        spline.bezier_points.add(len(bezier_points_data) - 1)
        for idx, pt in enumerate(bezier_points_data):
            bp = spline.bezier_points[idx]
            bp.co = pt["pos"]
            bp.handle_left = pt["handle_left"]
            bp.handle_right = pt["handle_right"]
            bp.handle_left_type = 'FREE'
            bp.handle_right_type = 'FREE'
            
            curve_obj[f"speed_{idx}"] = pt["speed"]
            curve_obj[f"event_{idx}"] = pt["event"]

        # 4. 敵オブジェクトを配置 (ゲームエンジン連動仕様に改良)
        for e_idx, enemy in enumerate(enemy_spawn_list):
            e_type = enemy["type"]
            empty_obj = bpy.data.objects.new(f"Enemy_{e_type}_Seg{enemy['seg']}_{e_idx}", None)
            empty_obj.empty_display_type = 'CUBE'
            empty_obj.empty_display_size = 2.0
            empty_obj.location = enemy["loc"]
            
            # level_editor.py で JSON 出力可能にするための設定
            empty_obj["file_name"] = e_type
            empty_obj["is_enemy"] = True
            empty_obj["enemy_type"] = e_type
            empty_obj["spawn_progress"] = enemy["spawn_progress"]
            empty_obj["parent_rail"] = curve_obj.name
            
            # アドオン側の flag のためのプロパティ
            try:
                empty_obj.is_enemy_flag = True
            except:
                pass
                
            bpy.context.scene.collection.objects.link(empty_obj)

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
        box.label(text="システム制約:")
        box.prop(props, "rail_length")
        box.prop(props, "start_z")
        box.prop(props, "min_z")
        
        layout.separator()
        layout.label(text="AIプロンプト:")
        layout.prop(props, "prompt", text="")
        
        layout.separator()
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