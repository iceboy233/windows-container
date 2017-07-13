$ErrorActionPreference = "Stop"
Get-ChildItem C:\Python* | ForEach-Object{
    $python = $_.Name
    if ($python -in 'Python35', 'Python35-x64', 'Python36', 'Python36-x64') {
        Write-Host ('Testing ' + $python) -ForegroundColor Magenta
        $originPath = $env:Path
        $env:Path = 'C:\' + $python + ';' + $env:Path
        Write-Host (python -c "print(__import__('sys').version)") -ForegroundColor Yellow

        python setup.py install
        if (-not $?) { throw }
        python -c 'import winc'
        if (-not $?) { throw }

        Write-Host ('Success ' + $python) -ForegroundColor Green
        $env:Path = $originPath
    }
}
