$ErrorActionPreference = 'Continue'
$src = 'C:\Users\16013\Desktop'
$dst = 'C:\Users\16013\Desktop\桌面整理_2026-09-03'

# 跳过项目：快捷方式、系统文件、隐藏配置目录
$skipFiles = @('desktop.ini')
$skipDirs  = @('.claude', '$RECYCLE.BIN', 'System Volume Information')

$dirMap = @(
    @{ Name = '一些开发资料';                 Dest = '01_课程学习' },
    @{ Name = 'SW6306资料';                   Dest = '02_SW6306项目' },
    @{ Name = 'sw6306_pdf';                   Dest = '02_SW6306项目' },
    @{ Name = '一些txt文件部分心得体会';        Dest = '02_SW6306项目' },
    @{ Name = 'SW6306V璧勬枡'; TargetName = 'SW6306V资料'; Dest = '02_SW6306项目' },
    @{ Name = 'ESP32P4开发、';                Dest = '03_ESP32与机械臂' },
    @{ Name = '机械臂';                       Dest = '03_ESP32与机械臂' },
    @{ Name = '客户_蛇需要建模推的需要改图纸';  Dest = '04_机械图纸' },
    @{ Name = '田禾技术有限公司';              Dest = '04_机械图纸' },
    @{ Name = '面试相关资料';                  Dest = '05_求职实习' },
    @{ Name = '实习记录';                     Dest = '05_求职实习' },
    @{ Name = '资料';                         Dest = '05_求职实习' },
    @{ Name = 'AI对话知识库';                 Dest = '06_AI对话' },
    @{ Name = 'Claude对话记录';               Dest = '06_AI对话' }
)

$fileMap = @(
    @{ Name = 'ESP32P4架构理解.md'; Dest = '03_ESP32与机械臂\散落文件' },
    @{ Name = 'xal1010 (1).pdf';    Dest = '07_待归档' },
    @{ Name = '~$BOM表.doc';        Dest = '07_待归档\临时文件' }
)

Write-Output '---------- 复制文件夹 ----------'
foreach ($t in $dirMap) {
    if ($skipDirs -contains $t.Name) { continue }
    $s = Join-Path $src $t.Name
    if (-not (Test-Path -LiteralPath $s)) {
        $alt = Get-ChildItem -LiteralPath $src -Directory -Force | Where-Object { $_.Name -like ($t.Name.Substring(0, 7) + '*') } | Select-Object -First 1
        if (-not $alt) { Write-Output ('[SKIP] 源不存在: ' + $t.Name); continue }
        $s = $alt.FullName
    }
    $leaf = if ($t.ContainsKey('TargetName')) { $t.TargetName } else { $t.Name }
    $d = Join-Path (Join-Path $dst $t.Dest) $leaf
    New-Item -ItemType Directory -Path $d -Force | Out-Null
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    robocopy "`"$s`"" "`"$d`"" /E /MT:16 /R:1 /W:1 /NFL /NDL /NJH /NJS /NP /XD '$RECYCLE.BIN' 'System Volume Information'
    $code = $LASTEXITCODE
    $sw.Stop()
    $ok = if ($code -lt 8) { 'OK' } else { 'FAIL' }
    Write-Output ('[' + $ok + '] exit=' + $code + '  ' + $t.Name + '  ->  ' + $t.Dest + '  ' + [math]::Round($sw.Elapsed.TotalSeconds, 1) + 's')
}

Write-Output '---------- 复制散落文件 ----------'
foreach ($t in $fileMap) {
    $s = Join-Path $src $t.Name
    if (-not (Test-Path -LiteralPath $s)) { Write-Output ('[SKIP] 源不存在: ' + $t.Name); continue }
    $d = Join-Path $dst $t.Dest
    New-Item -ItemType Directory -Path $d -Force | Out-Null
    try {
        Copy-Item -LiteralPath $s -Destination $d -Force
        Write-Output ('[OK] ' + $t.Name + '  ->  ' + $t.Dest)
    } catch {
        Write-Output ('[FAIL] ' + $t.Name + '  ' + $_.Exception.Message)
    }
}

Write-Output '---------- 校验（源文件数 vs 目标文件数）----------'
$rows = @()
foreach ($t in ($dirMap + $fileMap)) {
    $s = Join-Path $src $t.Name
    if (-not (Test-Path -LiteralPath $s)) {
        $alt = Get-ChildItem -LiteralPath $src -Directory -Force | Where-Object { $_.Name -like ($t.Name.Substring(0, 7) + '*') } | Select-Object -First 1
        if (-not $alt) { continue }
        $s = $alt.FullName
    }
    $leaf = if ($t.ContainsKey('TargetName')) { $t.TargetName } else { $t.Name }
    $d = Join-Path (Join-Path $dst $t.Dest) $leaf
    if (-not (Test-Path -LiteralPath $d)) { Write-Output ('[MISS] 目标缺失: ' + $leaf); continue }
    $sc = 0; $ssum = 0; $dc = 0; $dsum = 0
    if ((Get-Item -LiteralPath $s).PSIsContainer) {
        Get-ChildItem -LiteralPath $s -Recurse -File -Force -ErrorAction SilentlyContinue | ForEach-Object { $sc++; $ssum += $_.Length }
        Get-ChildItem -LiteralPath $d -Recurse -File -Force -ErrorAction SilentlyContinue | ForEach-Object { $dc++; $dsum += $_.Length }
    } else {
        $sc = 1; $ssum = (Get-Item -LiteralPath $s).Length; $dc = 1; $dsum = (Get-Item -LiteralPath $d).Length
    }
    $flag = if (($sc -eq $dc) -and ($ssum -eq $dsum)) { '一致' } else { '不一致!' }
    Write-Output ('  ' + $flag + '  源 ' + $sc + ' 个 / ' + [math]::Round($ssum / 1MB, 1) + 'MB   目标 ' + $dc + ' 个 / ' + [math]::Round($dsum / 1MB, 1) + 'MB   ' + $t.Name)
    $rows += [PSCustomObject]@{ 原名称 = $t.Name; 归入 = $t.Dest; 源文件数 = $sc; 源大小MB = [math]::Round($ssum / 1MB, 1); 目标文件数 = $dc; 目标大小MB = [math]::Round($dsum / 1MB, 1); 校验 = $flag }
}
$rows | Export-Csv -LiteralPath 'C:\Users\16013\Documents\ESP-IDF_projects\ESP32\tmp\manifest.csv' -NoTypeInformation -Encoding UTF8
Write-Output '=== 批次B 完成 ==='
