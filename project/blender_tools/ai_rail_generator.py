import bpy
import json
import urllib.request
import urllib.error
import math
import threading
import random

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
        default=50.0,
    )
    prompt: bpy.props.StringProperty(
        name="プロンプト",
        description="作成したいステージのイメージを自由に記述してください（例: 障害物を避ける激しいコース）",
        default="渓谷の中をすれすれでよけるような激しいコース",
    )

# 非同期通信用の状態管理
class AIGenState:
    is_running = False
    stage_data = None
    error_msg = None
    start_z = 0.0

def fetch_gemini_data(url, payload):
    try:
        data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
        with urllib.request.urlopen(req) as response:
            res_body = response.read().decode('utf-8')
            res_json = json.loads(res_body)
            generated_text = res_json['candidates'][0]['content']['parts'][0]['text']
            
            # AIの返答にマークダウン記法(```json ... ```)などが混ざっている場合に対処
            import re
            match = re.search(r'\{.*\}', generated_text, re.DOTALL)
            if match:
                generated_text = match.group(0)
                
            stage_data = json.loads(generated_text)
            AIGenState.stage_data = stage_data
    except Exception as e:
        import traceback
        traceback.print_exc()
        AIGenState.error_msg = str(e)
    finally:
        AIGenState.is_running = False

def check_ai_gen_thread():
    if AIGenState.is_running:
        return 0.5 # 0.5秒後に再チェック
        
    if AIGenState.error_msg:
        print("AI Gen Error:", AIGenState.error_msg)
        with open(r"c:\Users\k024g\Lesson\2025A\CG2\CG2\project\blender_tools\error.log", "w") as f:
            f.write("AI Gen Error:\n" + AIGenState.error_msg)
    elif AIGenState.stage_data:
        try:
            create_stage(AIGenState.stage_data, AIGenState.start_z)
            print("Stage elements generated successfully!")
        except Exception as e:
            import traceback
            traceback.print_exc()
            print("Create Stage Error:", e)
            with open(r"c:\Users\k024g\Lesson\2025A\CG2\CG2\project\blender_tools\error.log", "w") as f:
                f.write("Create Stage Error:\n")
                f.write(traceback.format_exc())
            
    return None # タイマー終了


def setup_geometry_nodes(terrain_obj, rail_obj):
    mod = terrain_obj.modifiers.new(name="AI_Terrain_Gen", type='NODES')
    node_group = bpy.data.node_groups.new(name="AI_Terrain_Nodes", type='GeometryNodeTree')
    mod.node_group = node_group
    
    try:
        node_group.interface.new_socket(name="Geometry", in_out='OUTPUT', socket_type='NodeSocketGeometry')
        node_group.interface.new_socket(name="Geometry", in_out='INPUT', socket_type='NodeSocketGeometry')
    except AttributeError:
        node_group.outputs.new('NodeSocketGeometry', "Geometry")
        node_group.inputs.new('NodeSocketGeometry', "Geometry")
        
    nodes = node_group.nodes
    links = node_group.links
    for node in nodes:
        nodes.remove(node)
        
    group_in = nodes.new('NodeGroupInput')
    group_in.location = (-1600, 0)
    
    group_out = nodes.new('NodeGroupOutput')
    group_out.location = (1600, 0)
    
    # ----------------------------------------------------
    # 1. レールの情報取得と細分化
    # ----------------------------------------------------
    obj_info = nodes.new('GeometryNodeObjectInfo')
    obj_info.inputs["Object"].default_value = rail_obj
    obj_info.transform_space = 'RELATIVE'
    obj_info.location = (-1200, 100)
    
    resample = nodes.new('GeometryNodeResampleCurve')
    resample.mode = 'LENGTH'
    resample.inputs["Length"].default_value = 5.0 # 5m間隔で細分化
    resample.location = (-1000, 100)
    
    # ----------------------------------------------------
    # 2. 道幅（Profile Curve）の作成
    # ----------------------------------------------------
    profile_line = nodes.new('GeometryNodeCurvePrimitiveLine')
    profile_line.inputs["Start"].default_value = (-1.0, 0.0, 0.0)
    profile_line.inputs["End"].default_value = (1.0, 0.0, 0.0)
    profile_line.location = (-1200, -100)
    
    profile_resample = nodes.new('GeometryNodeResampleCurve')
    profile_resample.mode = 'COUNT'
    profile_resample.inputs["Count"].default_value = 24 # 横幅の分割数
    profile_resample.location = (-1000, -100)
    
    # ----------------------------------------------------
    # 3. 中心からの距離をキャプチャ(0.0 ～ 1.0)
    # ----------------------------------------------------
    spline_param = nodes.new('GeometryNodeSplineParameter')
    spline_param.location = (-1200, -300)
    
    math_sub = nodes.new('ShaderNodeMath')
    math_sub.operation = 'SUBTRACT'
    math_sub.inputs[1].default_value = 0.5
    math_sub.location = (-1000, -300)
    
    math_abs = nodes.new('ShaderNodeMath')
    math_abs.operation = 'ABSOLUTE'
    math_abs.location = (-800, -300)
    
    math_mul = nodes.new('ShaderNodeMath')
    math_mul.operation = 'MULTIPLY'
    math_mul.inputs[1].default_value = 2.0
    math_mul.location = (-600, -300)
    
    capture = nodes.new('GeometryNodeCaptureAttribute')
    capture.domain = 'POINT'
    try:
        capture.data_type = 'FLOAT'
    except AttributeError:
        pass
    capture.location = (-800, -100)
    
    # ----------------------------------------------------
    # 4. メッシュ化（リボン生成）
    # ----------------------------------------------------
    # カーブのRadius（半径）が自動で適用され、セクションごとの幅が変わる
    curve_to_mesh = nodes.new('GeometryNodeCurveToMesh')
    curve_to_mesh.location = (-600, 100)
    
    # ----------------------------------------------------
    # 5. 崖の立ち上がり計算（Z軸への変位）
    # ----------------------------------------------------
    map_range = nodes.new('ShaderNodeMapRange')
    map_range.inputs[1].default_value = 0.5   # 中心から50%の幅までは谷底(平ら)
    map_range.inputs[2].default_value = 1.0   # 100%(端)で崖の頂上
    map_range.inputs[3].default_value = 0.0
    map_range.inputs[4].default_value = 150.0 # 崖の基本高さ(m)
    map_range.location = (-200, -300)
    
    # 崖の傾斜を急にする（指数関数）
    power = nodes.new('ShaderNodeMath')
    power.operation = 'POWER'
    power.inputs[1].default_value = 2.0
    power.location = (0, -300)
    
    # ----------------------------------------------------
    # 6. AIからの terrain_roughness に応じた岩肌ノイズ追加
    # ----------------------------------------------------
    named_attr = nodes.new('GeometryNodeInputNamedAttribute')
    try:
        named_attr.data_type = 'FLOAT'
    except AttributeError:
        pass
    named_attr.inputs["Name"].default_value = "terrain_roughness"
    named_attr.location = (-600, -500)
    
    pos = nodes.new('GeometryNodeInputPosition')
    pos.location = (-600, -700)
    
    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs["Scale"].default_value = 0.05
    noise.inputs["Detail"].default_value = 5.0
    noise.location = (-400, -700)
    
    noise_sub = nodes.new('ShaderNodeMath')
    noise_sub.operation = 'SUBTRACT'
    noise_sub.inputs[1].default_value = 0.3
    noise_sub.location = (-200, -700)
    
    # ノイズの強さ = roughness * MapRange(崖に近いほど岩肌が荒れる) * 50
    roughness_mul1 = nodes.new('ShaderNodeMath')
    roughness_mul1.operation = 'MULTIPLY'
    roughness_mul1.inputs[1].default_value = 50.0
    roughness_mul1.location = (0, -500)
    
    roughness_mul2 = nodes.new('ShaderNodeMath')
    roughness_mul2.operation = 'MULTIPLY'
    roughness_mul2.location = (200, -500)
    
    final_noise = nodes.new('ShaderNodeMath')
    final_noise.operation = 'MULTIPLY'
    final_noise.location = (400, -500)
    
    # ----------------------------------------------------
    # 7. 高さの合成とメッシュへの適用
    # ----------------------------------------------------
    final_height = nodes.new('ShaderNodeMath')
    final_height.operation = 'ADD'
    final_height.location = (600, -300)
    
    comb_xyz = nodes.new('ShaderNodeCombineXYZ')
    comb_xyz.location = (800, -300)
    
    set_pos = nodes.new('GeometryNodeSetPosition')
    set_pos.location = (1000, 100)
    
    shade_smooth = nodes.new('GeometryNodeSetShadeSmooth')
    shade_smooth.location = (1200, 100)
    
    # リンク接続
    links.new(obj_info.outputs["Geometry"], resample.inputs["Curve"])
    links.new(profile_line.outputs["Curve"], profile_resample.inputs["Curve"])
    
    links.new(spline_param.outputs["Factor"], math_sub.inputs[0])
    links.new(math_sub.outputs[0], math_abs.inputs[0])
    links.new(math_abs.outputs[0], math_mul.inputs[0])
    
    links.new(profile_resample.outputs["Curve"], capture.inputs[0])
    links.new(math_mul.outputs[0], capture.inputs[1])
    
    links.new(resample.outputs["Curve"], curve_to_mesh.inputs["Curve"])
    links.new(capture.outputs[0], curve_to_mesh.inputs["Profile Curve"])
    
    links.new(capture.outputs[1], map_range.inputs["Value"])
    links.new(map_range.outputs["Result"], power.inputs[0])
    
    links.new(pos.outputs["Position"], noise.inputs["Vector"])
    links.new(noise.outputs["Fac"], noise_sub.inputs[0])
    
    links.new(named_attr.outputs[0], roughness_mul1.inputs[0])
    links.new(roughness_mul1.outputs[0], roughness_mul2.inputs[0])
    links.new(capture.outputs[1], roughness_mul2.inputs[1])
    
    links.new(noise_sub.outputs[0], final_noise.inputs[0])
    links.new(roughness_mul2.outputs[0], final_noise.inputs[1])
    
    links.new(power.outputs[0], final_height.inputs[0])
    links.new(final_noise.outputs[0], final_height.inputs[1])
    links.new(final_height.outputs[0], comb_xyz.inputs["Z"])
    
    links.new(curve_to_mesh.outputs["Mesh"], set_pos.inputs["Geometry"])
    links.new(comb_xyz.outputs["Vector"], set_pos.inputs["Offset"])
    
    links.new(set_pos.outputs["Geometry"], shade_smooth.inputs["Geometry"])
    links.new(shade_smooth.outputs["Geometry"], group_out.inputs["Geometry"])


def create_stage(data, start_z):
    segments = data.get("segments", [])
    if not segments:
        return

    # 1. レール（Curve）オブジェクトのセットアップ (カメラレール用)
    curve_data = bpy.data.curves.new('AIRailCurve', type='CURVE')
    curve_data.dimensions = '3D'
    curve_obj = bpy.data.objects.new('AIRail', curve_data)
    bpy.context.scene.collection.objects.link(curve_obj)
    
    spline = curve_data.splines.new('BEZIER')
    
    current_pos = [0.0, 0.0, start_z]
    current_angle = math.radians(90.0)
    
    bezier_points_data = []

    first_seg = segments[0] if segments else {}
    terrain = first_seg.get("terrain", {})
    t_width = float(terrain.get("width", 20.0))
    t_roughness = float(terrain.get("roughness", 0.5))
    
    bezier_points_data.append({
        "pos": tuple(current_pos),
        "speed": float(first_seg.get("speed", 10.0)),
        "event": "START",
        "t_width": t_width,
        "t_roughness": t_roughness
    })

    for seg_idx, seg in enumerate(segments):
        raw_type = seg.get("type", "STRAIGHT")
        seg_type = str(raw_type).upper().replace("-", "_")
        
        length = float(seg.get("length", 50.0))
        speed = float(seg.get("speed", 12.0))
        
        terrain = seg.get("terrain", {})
        t_width = float(terrain.get("width", 20.0))
        t_roughness = float(terrain.get("roughness", 0.5))
        
        if seg_type in ["STRAIGHT", "CLIMB", "DIVE"]:
            z_offset = length * 0.3 if seg_type == "CLIMB" else (-length * 0.3 if seg_type == "DIVE" else 0.0)
            dx = length * math.cos(current_angle)
            dy = length * math.sin(current_angle)
            
            current_pos[0] += dx
            current_pos[1] += dy
            current_pos[2] += z_offset
            
            # 地面に潜らないように min_z でガード
            if current_pos[2] < bpy.context.scene.ai_rail_props.min_z:
                current_pos[2] = bpy.context.scene.ai_rail_props.min_z
                
            bezier_points_data.append({
                "pos": tuple(current_pos),
                "speed": speed,
                "event": seg_type,
                "t_width": t_width,
                "t_roughness": t_roughness
            })

        elif seg_type in ["CURVE_RIGHT", "CURVE_LEFT"]:
            is_right = (seg_type == "CURVE_RIGHT")
            radius = max(10.0, length)
            
            center_offset_angle = current_angle - math.radians(90.0) if is_right else current_angle + math.radians(90.0)
            cx = current_pos[0] + radius * math.cos(center_offset_angle)
            cy = current_pos[1] + radius * math.sin(center_offset_angle)
            
            steps = 6
            angle_step = math.radians(90.0) / steps
            
            start_arc_angle = current_angle + math.radians(90.0) if is_right else current_angle - math.radians(90.0)
            
            for step in range(1, steps + 1):
                if is_right:
                    arc_angle = start_arc_angle - angle_step * step
                    current_angle = arc_angle - math.radians(90.0)
                else:
                    arc_angle = start_arc_angle + angle_step * step
                    current_angle = arc_angle + math.radians(90.0)
                    
                current_pos[0] = cx + radius * math.cos(arc_angle)
                current_pos[1] = cy + radius * math.sin(arc_angle)
                
                bezier_points_data.append({
                    "pos": tuple(current_pos),
                    "speed": speed,
                    "event": f"{seg_type}_{step}",
                    "t_width": t_width,
                    "t_roughness": t_roughness
                })

    spline.bezier_points.add(len(bezier_points_data) - 1)

    # カスタム属性（terrain_roughness）をカーブに追加（Blenderのバージョンによっては非対応のため安全に処理）
    has_roughness_attr = False
    try:
        if hasattr(curve_data, "attributes"):
            if "terrain_roughness" not in curve_data.attributes:
                curve_data.attributes.new(name="terrain_roughness", type='FLOAT', domain='POINT')
            has_roughness_attr = True
    except Exception as e:
        print("Attribute Error (Ignored):", e)

    for idx, pt in enumerate(bezier_points_data):
        bp = spline.bezier_points[idx]
        bp.co = pt["pos"]
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
        
        # カーブの半径にAI指定の「width（幅）」を割り当てる
        bp.radius = pt.get("t_width", 20.0)
        
        curve_obj[f"speed_{idx}"] = pt["speed"]
        curve_obj[f"event_{idx}"] = pt["event"]
        
        # カスタム属性に roughness を保存
        if has_roughness_attr:
            try:
                curve_data.attributes['terrain_roughness'].data[idx].value = pt.get("t_roughness", 0.5)
            except Exception:
                pass

    # --- 2. 地形用オブジェクトのセットアップ ---
    terrain_mesh = bpy.data.meshes.new("AITerrainMesh")
    terrain_obj = bpy.data.objects.new("AITerrain", terrain_mesh)
    bpy.context.scene.collection.objects.link(terrain_obj)

    # 地形メッシュオブジェクトにGeometry Nodesを適用
    setup_geometry_nodes(terrain_obj, curve_obj)

    # 選択状態の更新
    bpy.context.view_layer.objects.active = curve_obj
    curve_obj.select_set(True)
    terrain_obj.select_set(True)


class AIRAIL_OT_Generate(bpy.types.Operator):
    bl_idname = "object.ai_rail_generate"
    bl_label = "Generate AI Rail"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        if AIGenState.is_running:
            self.report({'WARNING'}, "現在AIが生成中です... お待ちください。")
            return {'CANCELLED'}
            
        props = context.scene.ai_rail_props
        api_key = props.api_key
        
        if not api_key:
            self.report({'ERROR'}, "API Key is missing!")
            return {'CANCELLED'}
            
        AIGenState.is_running = True
        AIGenState.stage_data = None
        AIGenState.error_msg = None
        AIGenState.start_z = props.start_z
        
        auto_prompt = f"以下の条件に従って、全長およそ {props.rail_length}m のカメラレールセクション配列を生成してください。\n"
        auto_prompt += f"・レールの始点の高さ(Z座標): {props.start_z} m からスタートしてください。\n"
        auto_prompt += f"・【重要】Z座標(高さ)の下限: レールの高さは絶対に {props.min_z} mを下回らないように設計してください。\n"
        
        if props.prompt:
            auto_prompt += f"\n【ユーザーの希望するコースイメージ（この内容に沿って起伏やカーブを考えてください）】\n{props.prompt}\n"
        else:
            auto_prompt += "\n【ユーザーの希望するコースイメージ】\nおまかせでカッコいいコースを作ってください。\n"

        # 多様性アップのための裏テーマと乱数シード
        hidden_themes = [
            "起伏の激しい山岳地帯。急な上昇と下降を多く含めてください。",
            "鋭い右カーブと左カーブが連続する非常にテクニカルなコース。",
            "直線をメインにした超高速の駆け抜けコース。",
            "前半は直線、後半は複雑なカーブが続くドラマチックな展開。",
            "長い上昇のあとに一気に下降するジェットコースターのようなコース。"
        ]
        chosen_theme = random.choice(hidden_themes)
        random_seed = random.randint(0, 100000)
        
        auto_prompt += f"\n【AIへの隠し指示 (Random Seed: {random_seed})】\n今回のコースは「{chosen_theme}」というテーマを強調して、毎回異なる展開のセクション構成を生成してください。\n"

        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = f"""
あなたは「スターフォックス」や「パンツァードラグーン」のような名作3Dレールシューティングゲームの、超一流のレベルデザイナーです。
指定されたイメージに基づいて、ステージを構成する「セクション（レールの形状）」の並びと各セクションの「地形データ（ジオメトリノード用）」をJSON形式で出力してください。（現在敵の配置は行いません）

【出力JSONフォーマット】
必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。マークダウン(```json)や解説は一切含めないでください。

{{
  "segments": [
    {{
      "type": "STRAIGHT",
      "length": 50.0,
      "speed": 12.0,
      "terrain": {{
        "type": "OPEN",
        "width": 20.0,
        "roughness": 0.5,
        "obstacle_density": 0.2
      }}
    }}
  ]
}}

【セクションの種類 ("type")】
- "STRAIGHT": 直進するセクション。
- "CURVE_RIGHT": 右に90度旋回するセクション（lengthは旋回半径として扱われます。最低でも30以上の大きめの値を推奨）。
- "CURVE_LEFT": 左に90度旋回するセクション（lengthは旋回半径として扱われます。最低でも30以上の大きめの値を推奨）。
- "CLIMB": 上昇するセクション。
- "DIVE": 下降するセクション（Z軸下限 {props.min_z}m を下回らないよう注意）。

【地形データ ("terrain")】
ジオメトリノードでプロシージャル地形を生成するためのパラメータです。
- "type": "OPEN" (開けた地形/地面のみ) または "TUNNEL" (トンネル/洞窟)。
- "width": 地面の幅、またはトンネルの半径（10.0 ～ 50.0）。
- "roughness": 地形の起伏の激しさ（0.0: 平坦 ～ 1.0: 激しい）。
- "obstacle_density": 岩などの障害物の配置密度（0.0: なし ～ 1.0: 密集）。
"""
        # temperatureを高く設定して多様性を向上
        payload = {
            "contents": [{
                "parts": [{"text": system_prompt + "\n\nUser Request:\n" + auto_prompt}]
            }],
            "generationConfig": {
                "temperature": 1.5,
                "response_mime_type": "application/json"
            }
        }
        
        self.report({'INFO'}, "AIにリクエストを送信しました！バックグラウンドで生成中です... (Blenderを操作可能です)")
        
        # スレッド起動（フリーズ回避）
        thread = threading.Thread(target=fetch_gemini_data, args=(url, payload))
        thread.start()
        
        # タイマー登録
        if not bpy.app.timers.is_registered(check_ai_gen_thread):
            bpy.app.timers.register(check_ai_gen_thread)
        
        return {'FINISHED'}


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
        layout.operator(AIRAIL_OT_Generate.bl_idname, text="AIでステージ・地形を自動生成", icon='OUTLINER_OB_CURVE')
        
        if AIGenState.is_running:
            layout.label(text="🔄 AIがコースを生成中...", icon='TIME')

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
    
    if bpy.app.timers.is_registered(check_ai_gen_thread):
        bpy.app.timers.unregister(check_ai_gen_thread)

if __name__ == "__main__":
    register()