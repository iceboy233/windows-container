$ErrorActionPreference = "Stop"
Get-ChildItem C:\Python* | ForEach-Object{
    $python = $_.Name
    if ($python.StartsWith('Python35') -or $python.StartsWith('Python36')) {
        Write-Host ('Testing ' + $python) -ForegroundColor Magenta
        $originPath = $env:Path
        $env:Path = 'C:\' + $python + '\Scripts;' + 'C:\' + $python + ';' + $env:Path
        Write-Host (python -c "print(__import__('sys').version)") -ForegroundColor Yellow

        python .appveyor.py
        if (-not $?) { throw }

        Write-Host ('Success ' + $python) -ForegroundColor Green
        $env:Path = $originPath
    }
}
