# billding.obj の k = -1 スロット (curTrans.z = -59.05) でのワールド座標
# 自機 Z = 1410 .. 1580 付近
$lines = Get-Content "resources/3dModels/billding/billding.obj"
$results = @()

for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]
    if ($line -like "v *") {
        $parts = $line -split "\s+"
        $lx = [float]$parts[1]; $ly = [float]$parts[2]; $lz = [float]$parts[3]
        # node translation is (75.0987, 20.1921, -59.05)
        $wx = $lx + 75.0987
        $wy = $ly + 20.1921
        $wz = $lz - 59.0486
        if ($wz -ge 1410 -and $wz -le 1580) {
            $results += [PSCustomObject]@{
                Obj = "billding"
                X = [math]::Round($wx, 2)
                Y = [math]::Round($wy, 2)
                Z = [math]::Round($wz, 2)
            }
        }
    }
}

Write-Host "billding hits: $($results.Count)"
$results | Sort-Object Z | Format-Table -AutoSize
