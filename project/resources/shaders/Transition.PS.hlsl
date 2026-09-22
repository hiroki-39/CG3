struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> gRuleTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer TransitionBuffer : register(b0) {
    float4 fadeColor;      // トランジション色 (通常は純黒 {0, 0, 0, 1})
    float progress;        // 進行度 (0.0: 完全透明 〜 1.0: 完全に埋まる)
    float edgeSoftness;    // 境界アンチエイリアス幅
    int isOpening;         // 0 = 閉じる(イン: 左右端から中央へ■が拡大), 1 = 開く(アウト: 中央から左右端へ■が縮小)
    float4 edgeColor;      // タイルの枠線/エッジ色 (微細なアクセント)
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float2 uv = input.texcoord;

    // グリッド分割数 (1280x720 で各セル 40x40 ピクセルの正方形タイル)
    float2 grid = float2(32.0, 18.0);
    
    // セル座標 [0, grid)
    float2 cellIndex = floor(uv * grid);
    
    // セルのローカルUV [-0.5, 0.5] (セル中心が 0.0)
    float2 localUv = frac(uv * grid) - 0.5;
    
    // セルの中心座標 (UV空間 [0, 1])
    float2 cellCenter = (cellIndex + 0.5) / grid;
    
    // 左右端からの正規化距離: 左右端 (x=0, 1) で 0.0、画面中央 (x=0.5) で 1.0
    float distFromEdge = 1.0 - abs(cellCenter.x - 0.5) * 2.0;

    // タイルの拡縮スケール (0.0: 見えない 〜 1.0: セルを完全に埋める)
    float tileScale = 0.0;
    
    // 波の伝播の厚み
    float spread = 0.35;

    if (isOpening == 0) {
        // 【イン（閉じる時）】左右の画面端から中心に向かって■が拡大していく
        float start = distFromEdge * (1.0 - spread);
        float t = saturate((progress - start) / spread);
        tileScale = t * t * (3.0 - 2.0 * t);
    } else {
        // 【アウト（開く時）】画面中心から左右の端に向かって■が縮小していく
        float distFromCenter = abs(cellCenter.x - 0.5) * 2.0;
        float start = distFromCenter * (1.0 - spread);
        float t = saturate((progress - start) / spread);
        float ease = t * t * (3.0 - 2.0 * t);
        tileScale = 1.0 - ease;
    }

    // まだ拡大していないタイルは完全に破棄（元の画面を100%そのまま表示）
    if (tileScale <= 0.001) {
        discard;
    }

    // 正方形■の内側判定: max(|x|, |y|) <= halfSize
    // 最大時は 0.505 で隣接タイル間の隙間を防止
    float halfSize = tileScale * 0.505;
    float distToEdge = max(abs(localUv.x), abs(localUv.y));

    // ■の外側は即座に破棄（一切上書きせず、元のゲーム画面・タイトル画面を保持！）
    if (distToEdge >= halfSize) {
        discard;
    }

    // フチのアンチエイリアス（境界1ピクセルのみ滑らかに）
    float aa = 0.015;
    float alpha = smoothstep(halfSize, halfSize - aa, distToEdge);
    if (alpha <= 0.01) {
        discard;
    }

    // タイル色（基本は純黒）
    float3 finalColor = fadeColor.rgb;

    // 拡大中・縮小中のタイルの外枠ライン（サイバーエッジハイライト）
    float borderWidth = 0.03;
    if (distToEdge >= halfSize - borderWidth && tileScale > 0.05 && tileScale < 0.95) {
        finalColor = lerp(finalColor, edgeColor.rgb, edgeColor.a);
    }

    output.color = float4(finalColor, alpha * fadeColor.a);
    return output;
}
