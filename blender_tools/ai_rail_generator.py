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
    prompt: bpy.props.StringProperty(
        name="Prompt",
        description="Describe the rail you want to generate",
        default="長さ100mで、最初はゆっくり直線、途中で右にカーブして敵を出し、最後は加速するレール",
    )

class AIRAIL_OT_Generate(bpy.types.Operator):
    bl_idname = "object.ai_rail_generate"
    bl_label = "Generate AI Rail"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        props = context.scene.ai_rail_props
        api_key = props.api_key
        prompt = props.prompt

        if not api_key:
            self.report({'ERROR'}, "API Key is missing!")
            return {'CANCELLED'}

        # Gemini APIリクエストの準備
        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key={api_key.strip()}"
        
        system_prompt = """
あなたは3Dゲームのカメラレールを生成するAIです。指定された条件に基づいて、ベジェ曲線の制御点（位置、左ハンドル、右ハンドル）、速度、イベント情報をJSON形式で出力してください。
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
  ]
}
【重要ルール】
・レールの始点(1つ目の制御点)の座標は、必ず原点 (0.0, 0.0, 0.0) にしてください。
・レールが伸びる基本の進行方向は、必ず「+Y方向」(Y軸の正の方向) としてください。
・長さや曲がり具合は、+Y方向に進みながらX軸(左右)やZ軸(上下)を変化させて表現してください。
・ハンドルの座標は絶対座標(グローバル座標)で指定してください。進行方向が+Yなので、基本的なハンドルはY軸方向に伸ばす(+Yや-Y)形になります。
"""
        payload = {
            "contents": [{
                "parts": [{"text": system_prompt + "\n\nUser Request: " + prompt}]
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
        
        # 複数行のテキスト入力のように見せるために少し高さを取る
        layout.prop(props, "prompt", text="プロンプト")
        layout.operator(AIRAIL_OT_Generate.bl_idname, text="AIで生成する", icon='OUTLINER_OB_CURVE')

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
