$root = 'C:\Users\16013\Desktop'
$dirs = Get-ChildItem -LiteralPath $root -Force -Directory | Where-Object { $_.Name -ne '.claude' }
foreach ($d in $dirs) {
    Write-Output ('===== ' + $d.Name + ' =====')
    Get-ChildItem -LiteralPath $d.FullName -Force | Select-Object -First 25 | ForEach-Object {
        if ($_.PSIsContainer) { Write-Output ('  [D] ' + $_.Name) } else { Write-Output ('  [F] ' + $_.Name) }
    }
    $total = (Get-ChildItem -LiteralPath $d.FullName -Force | Measure-Object).Count
    if ($total -gt 25) { Write-Output ('  ... 共 ' + $total + ' 个直接子项') }
    Write-Output ''
}
Write-Output '===== 根目录散落文件 ====='
Get-ChildItem -LiteralPath $root -Force -File | ForEach-Object { Write-Output ('  ' + $_.Name) }
