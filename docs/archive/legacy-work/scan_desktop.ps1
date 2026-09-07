$root = 'C:\Users\16013\Desktop'
$out = Get-ChildItem -LiteralPath $root -Force | ForEach-Object {
    if ($_.PSIsContainer) {
        $sum = 0; $cnt = 0
        try {
            $e = [System.IO.Directory]::EnumerateFiles($_.FullName, '*', [System.IO.SearchOption]::AllDirectories)
            foreach ($f in $e) {
                $cnt++
                try { $sum += (New-Object System.IO.FileInfo($f)).Length } catch {}
            }
        } catch {}
        [PSCustomObject]@{ Kind = 'DIR'; Name = $_.Name; Files = $cnt; SizeMB = [math]::Round($sum / 1MB, 1) }
    } else {
        [PSCustomObject]@{ Kind = 'FILE'; Name = $_.Name; Files = 1; SizeMB = [math]::Round($_.Length / 1MB, 2) }
    }
}
$out | Sort-Object SizeMB -Descending | Format-Table -AutoSize | Out-String -Width 220
