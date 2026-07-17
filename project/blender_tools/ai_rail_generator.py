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
            stage_data = json.loads(generated_text)
            AIGenState.stage_data = stage_data
    except Exception as e:
        AIGenState.error_msg = str(e)
    finally:
        AIGenState.is_running = False

def check_ai_gen_thread():
    if AIGenState.is_running:
        return 0.5 # 0.5秒後に再チェック
        
    if AIGenState.error_msg:
        print("AI Gen Error:", AIGenState.error_msg)
    elif AIGenState.stage_data:
        try:
            create_stage(AIGenState.stage_data, AIGenState.start_z)
            print("Stage elements generated successfully!")
        except Exception as e:
            import traceback
            traceback.print_exc()
            print("Create Stage Error:", e)
            
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
    group_in.location = (-2400, 0)
    
    group_out = nodes.new('NodeGroupOutput')
    group_out.location = (1600, 0)
    
    # ----------------------------------------------------
    # 1. レールの情報を取得し、自動で最適なサイズのGridを作る
    # ----------------------------------------------------
    obj_info = nodes.new('GeometryNodeObjectInfo')
    obj_info.inputs["Object"].default_value = rail_obj
    obj_info.transform_space = 'RELATIVE' # 必須！位置ズレを防ぐ
    obj_info.location = (-2200, -200)
    
    # バウンディングボックスでレールの全体サイズと中心を取得
    bbox = nodes.new('GeometryNodeBoundBox')
    bbox.location = (-2000, -200)
    
    bbox_size = nodes.new('ShaderNodeVectorMath')
    bbox_size.operation = 'SUBTRACT'
    bbox_size.location = (-1800, -100)
    
    bbox_center_add = nodes.new('ShaderNodeVectorMath')
    bbox_center_add.operation = 'ADD'
    bbox_center_add.location = (-1800, -300)
    
    bbox_center = nodes.new('ShaderNodeVectorMath')
    bbox_center.operation = 'SCALE'
    bbox_center.inputs[3].default_value = 0.5
    bbox_center.location = (-1600, -300)
    
    # レールサイズに余白（崖の外側の広さ）を追加
    margin_add = nodes.new('ShaderNodeVectorMath')
    margin_add.operation = 'ADD'
    margin_add.inputs[1].default_value = (800.0, 800.0, 0.0) # 左右400mの余白
    margin_add.location = (-1600, -100)
    
    sep_size = nodes.new('ShaderNodeSeparateXYZ')
    sep_size.location = (-1400, -100)
    
    grid = nodes.new('GeometryNodeMeshGrid')
    grid.inputs["Vertices X"].default_value = 600
    grid.inputs["Vertices Y"].default_value = 600
    grid.location = (-1200, 100)
    
    # できたGridをレールの中心に移動
    grid_set_pos = nodes.new('GeometryNodeSetPosition')
    grid_set_pos.location = (-800, 100)
    
    sep_center = nodes.new('ShaderNodeSeparateXYZ')
    sep_center.location = (-1400, -400)
    comb_center = nodes.new('ShaderNodeCombineXYZ')
    comb_center.location = (-1200, -400)
    
    # リンク（Grid自動生成と配置）
    links.new(obj_info.outputs["Geometry"], bbox.inputs["Geometry"])
    links.new(bbox.outputs["Max"], bbox_size.inputs[0])
    links.new(bbox.outputs["Min"], bbox_size.inputs[1])
    links.new(bbox.outputs["Max"], bbox_center_add.inputs[0])
    links.new(bbox.outputs["Min"], bbox_center_add.inputs[1])
    links.new(bbox_center_add.outputs["Vector"], bbox_center.inputs[0])
    
    links.new(bbox_size.outputs["Vector"], margin_add.inputs[0])
    links.new(margin_add.outputs["Vector"], sep_size.inputs["Vector"])
    links.new(sep_size.outputs["X"], grid.inputs["Size X"])
    links.new(sep_size.outputs["Y"], grid.inputs["Size Y"])
    
    links.new(grid.outputs["Mesh"], grid_set_pos.inputs["Geometry"])
    links.new(bbox_center.outputs["Vector"], sep_center.inputs["Vector"])
    links.new(sep_center.outputs["X"], comb_center.inputs["X"])
    links.new(sep_center.outputs["Y"], comb_center.inputs["Y"])
    links.new(comb_center.outputs["Vector"], grid_set_pos.inputs["Offset"]) # Zは0のまま移動
    
    # ----------------------------------------------------
    # 2. レールから純粋な「XY平面の距離（2D距離）」を測る
    # ----------------------------------------------------
    curve_circle = nodes.new('GeometryNodeCurvePrimitiveCircle')
    curve_circle.inputs["Radius"].default_value = 1.0
    curve_circle.location = (-1200, 400)
    
    curve_to_mesh = nodes.new('GeometryNodeCurveToMesh')
    curve_to_mesh.location = (-1000, 300)
    
    # 距離計算の狂い（レールが空中にあると距離が遠くなる）を防ぐため、
    # 距離測定用レールのZ座標を強制的に0（ペシャンコ）にする
    set_rail_flat = nodes.new('GeometryNodeSetPosition')
    set_rail_flat.location = (-800, 300)
    
    pos_rail = nodes.new('GeometryNodeInputPosition')
    pos_rail.location = (-1200, 150)
    
    sep_rail = nodes.new('ShaderNodeSeparateXYZ')
    sep_rail.location = (-1000, 150)
    
    comb_rail = nodes.new('ShaderNodeCombineXYZ')
    comb_rail.location = (-800, 150) # Zは繋がないので0になる
    
    proximity = nodes.new('GeometryNodeProximity')
    proximity.target_element = 'FACES'
    proximity.location = (-600, 300)
    
    pos_node = nodes.new('GeometryNodeInputPosition')
    pos_node.location = (-800, 0)
    
    links.new(obj_info.outputs["Geometry"], curve_to_mesh.inputs["Curve"])
    links.new(curve_circle.outputs["Curve"], curve_to_mesh.inputs["Profile Curve"])
    links.new(curve_to_mesh.outputs["Mesh"], set_rail_flat.inputs["Geometry"])
    
    # ペシャンコ化のリンク
    links.new(pos_rail.outputs["Position"], sep_rail.inputs["Vector"])
    links.new(sep_rail.outputs["X"], comb_rail.inputs["X"])
    links.new(sep_rail.outputs["Y"], comb_rail.inputs["Y"])
    links.new(comb_rail.outputs["Vector"], set_rail_flat.inputs["Position"])
    
    links.new(set_rail_flat.outputs["Geometry"], proximity.inputs["Target"])
    links.new(pos_node.outputs["Position"], proximity.inputs["Source Position"])
    
    # ----------------------------------------------------
    # 3. 崖の立ち上がり計算（絶壁）
    # ----------------------------------------------------
    map_range = nodes.new('ShaderNodeMapRange')
    map_range.inputs[1].default_value = 30.0  # 谷底の幅（レールから30m）
    map_range.inputs[2].default_value = 70.0  # 崖の上の幅（70m地点で頂上）
    map_range.inputs[3].default_value = 0.0
    map_range.inputs[4].default_value = 1.0
    map_range.location = (-400, 300)
    
    power = nodes.new('ShaderNodeMath')
    power.operation = 'POWER'
    power.inputs[1].default_value = 0.2 # 強烈な急勾配
    power.location = (-200, 300)
    
    cliff_height = nodes.new('ShaderNodeMath')
    cliff_height.operation = 'MULTIPLY'
    cliff_height.inputs[1].default_value = 200.0 # 崖の基本高さ
    cliff_height.location = (0, 300)
    
    links.new(proximity.outputs["Distance"], map_range.inputs["Value"])
    links.new(map_range.outputs["Result"], power.inputs[0])
    links.new(power.outputs["Value"], cliff_height.inputs[0])
    
    # ----------------------------------------------------
    # 4. 岩肌と浸食のノイズ（レール部分はノイズ0にする賢いマスク処理）
    # ----------------------------------------------------
    surface_noise = nodes.new('ShaderNodeTexNoise')
    surface_noise.inputs["Scale"].default_value = 0.02
    surface_noise.inputs["Detail"].default_value = 6.0
    surface_noise.location = (-400, -100)
    
    surf_sub = nodes.new('ShaderNodeMath')
    surf_sub.operation = 'SUBTRACT'
    surf_sub.inputs[1].default_value = 0.3 # 隆起させる
    surf_sub.location = (-200, -100)
    
    surf_mul = nodes.new('ShaderNodeMath')
    surf_mul.operation = 'MULTIPLY'
    surf_mul.inputs[1].default_value = 150.0 # 激しい岩肌の起伏（最大150m）
    surf_mul.location = (0, -100)
    
    # マスク処理：MapRangeの結果(0~1)を掛けることで、谷底(0)はノイズ無効、崖の上(1)は激しいノイズになる
    noise_mask = nodes.new('ShaderNodeMath')
    noise_mask.operation = 'MULTIPLY'
    noise_mask.location = (200, -100)
    
    links.new(pos_node.outputs["Position"], surface_noise.inputs["Vector"])
    links.new(surface_noise.outputs["Fac"], surf_sub.inputs[0])
    links.new(surf_sub.outputs["Value"], surf_mul.inputs[0])
    
    links.new(surf_mul.outputs["Value"], noise_mask.inputs[0])
    links.new(map_range.outputs["Result"], noise_mask.inputs[1]) # ここがミソ！
    
    # ----------------------------------------------------
    # 5. 高さの合成と地形の適用
    # ----------------------------------------------------
    final_add = nodes.new('ShaderNodeMath')
    final_add.operation = 'ADD'
    final_add.location = (400, 100)
    
    comb_z = nodes.new('ShaderNodeCombineXYZ')
    comb_z.location = (600, 100)
    
    set_pos_final = nodes.new('GeometryNodeSetPosition')
    set_pos_final.location = (800, 100)
    
    shade_smooth = nodes.new('GeometryNodeSetShadeSmooth')
    shade_smooth.location = (1000, 100)
    
    links.new(cliff_height.outputs["Value"], final_add.inputs[0])
    links.new(noise_mask.outputs["Value"], final_add.inputs[1])
    links.new(final_add.outputs["Value"], comb_z.inputs["Z"])
    
    links.new(grid_set_pos.outputs["Geometry"], set_pos_final.inputs["Geometry"])
    links.new(comb_z.outputs["Vector"], set_pos_final.inputs["Offset"])
    
    links.new(set_pos_final.outputs["Geometry"], shade_smooth.inputs["Geometry"])
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
    bezier_points_data.append({
        "pos": tuple(current_pos),
        "speed": float(first_seg.get("speed", 10.0)),
        "event": "START"
    })

    for seg_idx, seg in enumerate(segments):
        seg_type = seg.get("type", "STRAIGHT")
        length = float(seg.get("length", 50.0))
        speed = float(seg.get("speed", 12.0))
        
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
                "event": seg_type
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
                    "event": f"{seg_type}_{step}"
                })

    spline.bezier_points.add(len(bezier_points_data) - 1)

    for idx, pt in enumerate(bezier_points_data):
        bp = spline.bezier_points[idx]
        bp.co = pt["pos"]
        bp.handle_left_type = 'AUTO'
        bp.handle_right_type = 'AUTO'
        
        curve_obj[f"speed_{idx}"] = pt["speed"]
        curve_obj[f"event_{idx}"] = pt["event"]

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