$ErrorActionPreference = 'Continue'
$src = 'C:\Users\16013\Desktop'
$dst = 'C:\Users\16013\Desktop\桌面整理_2026-09-03'
$tmp = 'C:\Users\16013\Documents\ESP-IDF_projects\ESP32\tmp'

$big = @(
    @{ Name = '作业+心得+备忘录+资料'; Dest = '01_课程学习' },
    @{ Name = '2026-第2学期相关文档';   Dest = '01_课程学习' }
)

Write-Output '---------- 补验大目录 ----------'
$bigRows = @()
foreach ($t in $big) {
    $s = Join-Path $src $t.Name
    $d = Join-Path (Join-Path $dst $t.Dest) $t.Name
    if (-not (Test-Path -LiteralPath $d)) { Write-Output ('[MISS] ' + $t.Name); continue }
    $sc = 0; $ssum = 0; $dc = 0; $dsum = 0
    Get-ChildItem -LiteralPath $s -Recurse -File -Force -ErrorAction SilentlyContinue | ForEach-Object { $sc++; $ssum += $_.Length }
    Get-ChildItem -LiteralPath $d -Recurse -File -Force -ErrorAction SilentlyContinue | ForEach-Object { $dc++; $dsum += $_.Length }
    $flag = if (($sc -eq $dc) -and ($ssum -eq $dsum)) { '一致' } else { '不一致!' }
    Write-Output ('  ' + $flag + '  源 ' + $sc + ' 个 / ' + [math]::Round($ssum / 1MB, 1) + 'MB   目标 ' + $dc + ' 个 / ' + [math]::Round($dsum / 1MB, 1) + 'MB   ' + $t.Name)
    $bigRows += [PSCustomObject]@{ 原名称 = $t.Name; 归入 = $t.Dest; 源文件数 = $sc; 源大小MB = [math]::Round($ssum / 1MB, 1); 目标文件数 = $dc; 目标大小MB = [math]::Round($dsum / 1MB, 1); 校验 = $flag }
}

Write-Output '---------- 整理区总览 ----------'
$all = Get-ChildItem -LiteralPath $dst -Recurse -File -Force -ErrorAction SilentlyContinue
$totalFiles = ($all | Measure-Object).Count
$totalMB = [math]::Round((($all | Measure-Object -Property Length -Sum).Sum) / 1MB, 1)
Write-Output ('  整理区合计: ' + $totalFiles + ' 个文件 / ' + $totalMB + ' MB')

$catRows = Get-ChildItem -LiteralPath $dst -Directory -Force | ForEach-Object {
    $c = Get-ChildItem -LiteralPath $_.FullName -Recurse -File -Force -ErrorAction SilentlyContinue
    $n = ($c | Measure-Object).Count
    $m = [math]::Round((($c | Measure-Object -Property Length -Sum).Sum) / 1MB, 1)
    [PSCustomObject]@{ 分类 = $_.Name; 文件数 = $n; 大小MB = $m }
}
$catRows | ForEach-Object { Write-Output ('  ' + $_.分类 + '   ' + $_.文件数 + ' 个 / ' + $_.大小MB + ' MB') }

# 合并批次B的校验结果
$bRows = @()
if (Test-Path -LiteralPath (Join-Path $tmp 'manifest.csv')) {
    $bRows = Import-Csv -LiteralPath (Join-Path $tmp 'manifest.csv') -Encoding UTF8
}
$allRows = @($bigRows) + @($bRows)

Write-Output '---------- 生成清单 ----------'
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('# 桌面整理清单')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('生成时间：2026-09-03')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('> **本目录是复制结果。桌面上所有原件均未删除、未移动、未改名。**')
[void]$sb.AppendLine('> 请你逐项核对，确认无误后，自行删除桌面原件。')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('## 一、目录结构')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('```')
[void]$sb.AppendLine('桌面整理_2026-09-03/')
[void]$sb.AppendLine('├── 01_课程学习/')
[void]$sb.AppendLine('│   ├── 作业+心得+备忘录+资料')
[void]$sb.AppendLine('│   ├── 2026-第2学期相关文档')
[void]$sb.AppendLine('│   └── 一些开发资料')
[void]$sb.AppendLine('├── 02_SW6306项目/')
[void]$sb.AppendLine('│   ├── SW6306资料')
[void]$sb.AppendLine('│   ├── sw6306_pdf')
[void]$sb.AppendLine('│   ├── SW6306V资料            （原名乱码，已重命名）')
[void]$sb.AppendLine('│   └── 一些txt文件部分心得体会')
[void]$sb.AppendLine('├── 03_ESP32与机械臂/')
[void]$sb.AppendLine('│   ├── ESP32P4开发、')
[void]$sb.AppendLine('│   ├── 机械臂')
[void]$sb.AppendLine('│   └── 散落文件/ESP32P4架构理解.md')
[void]$sb.AppendLine('├── 04_机械图纸/')
[void]$sb.AppendLine('│   ├── 客户_蛇需要建模推的需要改图纸')
[void]$sb.AppendLine('│   └── 田禾技术有限公司')
[void]$sb.AppendLine('├── 05_求职实习/')
[void]$sb.AppendLine('│   ├── 面试相关资料')
[void]$sb.AppendLine('│   ├── 实习记录')
[void]$sb.AppendLine('│   └── 资料')
[void]$sb.AppendLine('├── 06_AI对话/')
[void]$sb.AppendLine('│   ├── AI对话知识库')
[void]$sb.AppendLine('│   └── Claude对话记录')
[void]$sb.AppendLine('└── 07_待归档/')
[void]$sb.AppendLine('    └── xal1010 (1).pdf')
[void]$sb.AppendLine('```')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('## 二、分类统计')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('| 分类 | 文件数 | 大小(MB) |')
[void]$sb.AppendLine('|---|---:|---:|')
foreach ($r in $catRows) {
    [void]$sb.AppendLine(('| ' + $r.分类 + ' | ' + $r.文件数 + ' | ' + $r.大小MB + ' |'))
}
[void]$sb.AppendLine(('| **合计** | **' + $totalFiles + '** | **' + $totalMB + '** |'))
[void]$sb.AppendLine('')
[void]$sb.AppendLine('## 三、逐项校验（源文件数/体积 vs 副本）')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('| 原名称 | 归入 | 源文件数 | 源大小(MB) | 副本文件数 | 副本大小(MB) | 校验 |')
[void]$sb.AppendLine('|---|---|---:|---:|---:|---:|---|')
foreach ($r in $allRows) {
    [void]$sb.AppendLine(('| ' + $r.原名称 + ' | ' + $r.归入 + ' | ' + $r.源文件数 + ' | ' + $r.源大小MB + ' | ' + $r.目标文件数 + ' | ' + $r.目标大小MB + ' | ' + $r.校验 + ' |'))
}
[void]$sb.AppendLine('')
[void]$sb.AppendLine('## 四、未纳入整理的内容')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('| 项目 | 说明 |')
[void]$sb.AppendLine('|---|---|')
[void]$sb.AppendLine('| 15 个 `.lnk` 快捷方式 | 按你的要求不整理，留在桌面方便直接启动软件 |')
[void]$sb.AppendLine('| `desktop.ini` | 系统文件，不复制 |')
[void]$sb.AppendLine('| `.claude` 隐藏目录 | 工具配置目录，保留原地 |')
[void]$sb.AppendLine('| `esp32-lvgl-learning-master` | 你已确认并自行删除 |')
[void]$sb.AppendLine('| `~$BOM表.doc` | Word 临时锁文件，复制后已被 Word 自动清理（正文 BOM表.doc 不在桌面上） |')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('## 五、需要你留意')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('1. **`SW6306V资料`**：原目录名显示为 `SW6306V璧勬枡`（编码乱码，实际是"SW6306V资料"），副本已改为正确名称；该目录扫描结果为 **0 个文件**（仅有一个空的 `pdf_pages` 子目录）。')
[void]$sb.AppendLine('2. **`03_ESP32与机械臂/机械臂`**：含 `.pio`、`build`、`dist` 等编译产物目录，体积 75.6MB，如需精简可自行清理副本中的构建缓存。')
[void]$sb.AppendLine('3. **`01_课程学习/作业+心得+备忘录+资料`**：5GB，含 `projects`、`coursework` 等，可能有 `node_modules` / `.git` 等依赖目录，本次按你的要求全量复制，未做任何排除。')
[void]$sb.AppendLine('4. 副本与原件内容一致（文件数与总字节数逐项比对通过），但**未做逐文件哈希校验**。')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('## 六、核对完成后')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('确认副本无误后，桌面上这些原件需要你手动删除（本次未执行任何删除操作）：')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('- `作业+心得+备忘录+资料`、`2026-第2学期相关文档`、`一些开发资料`')
[void]$sb.AppendLine('- `SW6306资料`、`sw6306_pdf`、`SW6306V璧勬枡`、`一些txt文件部分心得体会`')
[void]$sb.AppendLine('- `ESP32P4开发、`、`机械臂`、`ESP32P4架构理解.md`')
[void]$sb.AppendLine('- `客户_蛇需要建模推的需要改图纸`、`田禾技术有限公司`')
[void]$sb.AppendLine('- `面试相关资料`、`实习记录`、`资料`')
[void]$sb.AppendLine('- `AI对话知识库`、`Claude对话记录`')
[void]$sb.AppendLine('- `xal1010 (1).pdf`')
[void]$sb.AppendLine('')

$outFile = Join-Path $dst '整理清单.md'
[System.IO.File]::WriteAllText($outFile, $sb.ToString(), [System.Text.UTF8Encoding]::new($false))
Write-Output ('清单已生成: ' + $outFile)
Write-Output '=== 全部完成 ==='
