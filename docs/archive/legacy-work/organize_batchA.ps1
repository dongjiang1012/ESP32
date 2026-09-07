$ErrorActionPreference = 'Continue'
$src = 'C:\Users\16013\Desktop'
$dst = 'C:\Users\16013\Desktop\桌面整理_2026-09-03'
$logDir = 'C:\Users\16013\Documents\ESP-IDF_projects\ESP32\tmp'
if (-not (Test-Path -LiteralPath $logDir)) { New-Item -ItemType Directory -Path $logDir -Force | Out-Null }

$targets = @(
    @{ Name = '作业+心得+备忘录+资料'; Dest = '01_课程学习' },
    @{ Name = '2026-第2学期相关文档';     Dest = '01_课程学习' }
)

foreach ($t in $targets) {
    $s = Join-Path $src $t.Name
    $d = Join-Path (Join-Path $dst $t.Dest) $t.Name
    if (-not (Test-Path -LiteralPath $s)) {
        Write-Output ('[SKIP] 源不存在: ' + $t.Name)
        continue
    }
    New-Item -ItemType Directory -Path $d -Force | Out-Null
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    robocopy "`"$s`"" "`"$d`"" /E /MT:16 /R:1 /W:1 /NFL /NDL /NJH /NJS /NP /XD '$RECYCLE.BIN' 'System Volume Information'
    $code = $LASTEXITCODE
    $sw.Stop()
    $ok = if ($code -lt 8) { 'OK' } else { 'FAIL' }
    Write-Output ('[' + $ok + '] exit=' + $code + '  ' + $t.Name + '  ->  ' + $t.Dest + '  用时 ' + [math]::Round($sw.Elapsed.TotalSeconds, 1) + 's')
}
Write-Output '=== 批次A 完成 ==='
