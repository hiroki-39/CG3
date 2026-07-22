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
    prompt_early: bpy.props.StringProperty(
        name="序盤のイメージ",
        description="ステージ序盤（スタート直後）のイメージ",
        default="平原を駆け抜ける高速コース",
    )
    prompt_mid: bpy.props.StringProperty(
        name="中盤のイメージ",
        description="ステージ中盤のイメージ",
        default="徐々にビル群が見え始め、市街地へ突入する",
    )
    prompt_late: bpy.props.StringProperty(
        name="終盤のイメージ",
        description="ステージ終盤（ボス前など）のイメージ",
        default="密集した高層ビル群を縫うように飛ぶ激しいコース",
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


def get_or_create_module(module_name):
    obj_name = f"Module_{module_name}"
    if obj_name in bpy.data.objects:
        return bpy.data.objects[obj_name]
    
    # 見つからない場合はダミーを作成
    if module_name == "BUILDING":
        bpy.ops.mesh.primitive_cube_add(size=1)
        obj = bpy.context.active_object
        obj.scale = (random.uniform(10, 30), random.uniform(10, 30), random.uniform(50, 150))
    elif module_name == "ROCK":
        bpy.ops.mesh.primitive_ico_sphere_add(radius=10, subdivisions=2)
        obj = bpy.context.active_object
        obj.scale = (random.uniform(1.5, 4.0), random.uniform(1.5, 4.0), random.uniform(1.5, 5.0))
    elif module_name == "TREE":
        bpy.ops.mesh.primitive_cylinder_add(radius=2, depth=20)
        obj = bpy.context.active_object
    else:
        # AIが未知のモジュールを出力した場合のフォールバック
        bpy.ops.mesh.primitive_cube_add(size=20)
        obj = bpy.context.active_object
        obj.scale = (1, 1, random.uniform(1, 3))
        
    obj.name = obj_name
    # オリジナルは非表示
    obj.hide_render = True
    obj.hide_viewport = True
    
    col_name = "AI_Modules_Source"
    if col_name not in bpy.data.collections:
        col = bpy.data.collections.new(col_name)
        bpy.context.scene.collection.children.link(col)
    else:
        col = bpy.data.collections[col_name]
        
    for c in obj.users_collection:
        c.objects.unlink(obj)
    col.objects.link(obj)
    
    return obj

def scatter_at_point(px, py, pz, angle, width, modules, density, target_collection):
    # 左右両方に配置を試みる
    for side in [-1, 1]:
        # 密度による間引き（密度が高いほど配置されやすい）
        if random.random() > density:
            continue
            
        # さらに、密度が高い場合は同じ側にも複数個（壁のように）厚みを持たせて配置する
        num_clusters = 1
        if density > 0.7:
            num_clusters = random.randint(1, 3)
            
        for _ in range(num_clusters):
            # 道幅(width)の外側に配置する（道幅の半分 + ランダムなオフセット）
            offset_dist = (width * 0.5) + random.uniform(2.0, 40.0)
            
            # angleは進行方向。横方向は +90度
            side_angle = angle + math.radians(90.0) if side == 1 else angle - math.radians(90.0)
            
            spawn_x = px + offset_dist * math.cos(side_angle)
            spawn_y = py + offset_dist * math.sin(side_angle)
            
            # Z座標は地形の起伏に合わせるなど工夫できるが、一旦レールと同じか少し下げる
            spawn_z = pz - random.uniform(0.0, 15.0)
            
            mod_name = random.choice(modules)
            source_obj = get_or_create_module(mod_name)
            
            # リンク複製 (Alt+D)
            new_obj = source_obj.copy()
            new_obj.data = source_obj.data # データ共有（念のため）
            new_obj.hide_render = False
            new_obj.hide_viewport = False
            new_obj.name = f"Gen_{mod_name}"
            
            new_obj.location = (spawn_x, spawn_y, spawn_z)
            new_obj.rotation_euler = (0, 0, random.uniform(0, math.pi * 2))
            
            # スケールも少しばらけさせる
            s = random.uniform(0.7, 1.4)
            new_obj.scale = (source_obj.scale[0]*s, source_obj.scale[1]*s, source_obj.scale[2]*s)
            
            target_collection.objects.link(new_obj)


def create_stage(data, start_z):
    early_segments = data.get("early_segments", [])
    mid_segments = data.get("mid_segments", [])
    late_segments = data.get("late_segments", [])
    
    # AIが古いフォーマット(segments)で返してきた場合のフォールバック
    if not early_segments and not mid_segments and not late_segments:
        early_segments = data.get("segments", [])
        if not early_segments:
            return

    # --- 1. 出力用コレクションの準備 ---
    stage_col_name = "AI_Generated_Stage"
    if stage_col_name in bpy.data.collections:
        stage_col = bpy.data.collections[stage_col_name]
    else:
        stage_col = bpy.data.collections.new(stage_col_name)
        bpy.context.scene.collection.children.link(stage_col)

    current_pos = [0.0, 0.0, start_z]
    current_angle = math.radians(90.0)
    
    phases = [
        ("Early", early_segments),
        ("Mid", mid_segments),
        ("Late", late_segments)
    ]
    
    active_curves = []

    for phase_name, segments in phases:
        if not segments:
            continue
            
        curve_data = bpy.data.curves.new(f'AIRail_{phase_name}', type='CURVE')
        curve_data.dimensions = '3D'
        curve_obj = bpy.data.objects.new(f'AIRail_{phase_name}', curve_data)
        stage_col.objects.link(curve_obj)
        active_curves.append(curve_obj)
        
        spline = curve_data.splines.new('BEZIER')
        
        bezier_points_data = []

        first_seg = segments[0]
        bezier_points_data.append({
            "pos": tuple(current_pos),
            "speed": float(first_seg.get("speed", 10.0)),
            "event": f"START_{phase_name.upper()}"
        })

        # レール生成と並行してモジュールを配置
        for seg_idx, seg in enumerate(segments):
            seg_type = seg.get("type", "STRAIGHT")
            length = float(seg.get("length", 50.0))
            speed = float(seg.get("speed", 12.0))
            
            terrain_data = seg.get("terrain", {})
            modules = terrain_data.get("modules", [])
            density = float(terrain_data.get("obstacle_density", 0.0))
            width = float(terrain_data.get("width", 30.0))
            
            prev_pos = list(current_pos)
            
            if seg_type in ["STRAIGHT", "CLIMB", "DIVE"]:
                z_offset = length * 0.3 if seg_type == "CLIMB" else (-length * 0.3 if seg_type == "DIVE" else 0.0)
                dx = length * math.cos(current_angle)
                dy = length * math.sin(current_angle)
                
                current_pos[0] += dx
                current_pos[1] += dy
                current_pos[2] += z_offset
                
                if current_pos[2] < bpy.context.scene.ai_rail_props.min_z:
                    current_pos[2] = bpy.context.scene.ai_rail_props.min_z
                    
                bezier_points_data.append({
                    "pos": tuple(current_pos),
                    "speed": speed,
                    "event": seg_type
                })
                
                # --- モジュール散布 (直線) ---
                if modules and density > 0:
                    step_size = max(5.0, 30.0 * (1.0 - density))
                    num_steps = max(1, int(length / step_size))
                    for i in range(num_steps):
                        t = i / num_steps
                        px = prev_pos[0] + dx * t
                        py = prev_pos[1] + dy * t
                        pz = prev_pos[2] + z_offset * t
                        scatter_at_point(px, py, pz, current_angle, width, modules, density, stage_col)

            elif seg_type in ["CURVE_RIGHT", "CURVE_LEFT"]:
                is_right = (seg_type == "CURVE_RIGHT")
                radius = max(10.0, length)
                
                center_offset_angle = current_angle - math.radians(90.0) if is_right else current_angle + math.radians(90.0)
                cx = prev_pos[0] + radius * math.cos(center_offset_angle)
                cy = prev_pos[1] + radius * math.sin(center_offset_angle)
                
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
                    
                    # --- モジュール散布 (カーブ) ---
                    if modules and density > 0:
                        scatter_at_point(current_pos[0], current_pos[1], current_pos[2], current_angle, width, modules, density, stage_col)

        spline.bezier_points.add(len(bezier_points_data) - 1)

        for idx, pt in enumerate(bezier_points_data):
            bp = spline.bezier_points[idx]
            bp.co = pt["pos"]
            bp.handle_left_type = 'AUTO'
            bp.handle_right_type = 'AUTO'
            
            curve_obj[f"speed_{idx}"] = pt["speed"]
            curve_obj[f"event_{idx}"] = pt["event"]

    # 選択状態の更新
    if active_curves:
        bpy.context.view_layer.objects.active = active_curves[0]
        for c in active_curves:
            c.select_set(True)


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
        
        auto_prompt = f"以下の条件に従って、序盤・中盤・終盤の3つのセクション配列を生成してください。\n"
        auto_prompt += f"・各フェーズ（序盤、中盤、終盤）ごとに、それぞれおよそ {props.rail_length}m になるようにセクションを構成してください。（全体で {props.rail_length * 3}m）\n"
        auto_prompt += f"・レールの始点の高さ(Z座標): {props.start_z} m からスタートしてください。\n"
        auto_prompt += f"・【重要】Z座標(高さ)の下限: レールの高さは絶対に {props.min_z} mを下回らないように設計してください。\n"
        
        auto_prompt += "\n【ユーザーの希望するコースの進行イメージ（3段階）】\n"
        auto_prompt += f"序盤（ステージ開始〜）: {props.prompt_early}\n"
        auto_prompt += f"中盤（ステージ中盤〜）: {props.prompt_mid}\n"
        auto_prompt += f"終盤（ボス前やラスト）: {props.prompt_late}\n"
        auto_prompt += "\nこれら3つのイメージが滑らかに移行するように、それぞれ early_segments, mid_segments, late_segments の配列に分けて出力してください。\n"

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
指定されたイメージに基づいて、ステージを構成する「セクション（レールの形状）」の並びと、各セクションの「モジュール配置データ」をJSON形式で出力してください。（現在敵の配置は行いません）

【出力JSONフォーマット】
必ず以下のJSONスキーマに従い、JSONテキストのみを出力してください。マークダウン(```json)や解説は一切含めないでください。

{{
  "early_segments": [
    {{
      "type": "STRAIGHT",
      "length": 50.0,
      "speed": 12.0,
      "terrain": {{
        "type": "OPEN",
        "modules": ["BUILDING", "ROCK"],
        "obstacle_density": 0.5,
        "width": 30.0
      }}
    }}
  ],
  "mid_segments": [],
  "late_segments": []
}}

【セクションの種類 ("type")】
- "STRAIGHT": 直進するセクション。
- "CURVE_RIGHT": 右に90度旋回するセクション（lengthは旋回半径として扱われます。最低でも30以上の大きめの値を推奨）。
- "CURVE_LEFT": 左に90度旋回するセクション（lengthは旋回半径として扱われます。最低でも30以上の大きめの値を推奨）。
- "CLIMB": 上昇するセクション。
- "DIVE": 下降するセクション（Z軸下限 {props.min_z}m を下回らないよう注意）。

【地形・モジュールデータ ("terrain")】
- "type": "OPEN" (開けた地形)、"TUNNEL" (トンネル/洞窟)、"CITY" (市街地)、"TRENCH" (人工の溝・通路)。
- "modules": この区間で配置する障害物や景観パーツのリスト。以下の文字列から0個以上選んでください。
    - "BUILDING" (ビル・建物)
    - "ROCK" (岩・隕石)
    - "TREE" (木・植物)
    - "RUIN" (遺跡・瓦礫)
- "obstacle_density": モジュールの配置密度（0.0: なし ～ 1.0: 密集）。
- "width": 空間の広さ（コース幅）。（10.0 ～ 50.0）。
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
        layout.label(text="AIプロンプト（ステージ構成）:")
        layout.prop(props, "prompt_early", text="序盤")
        layout.prop(props, "prompt_mid", text="中盤")
        layout.prop(props, "prompt_late", text="終盤")
        
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