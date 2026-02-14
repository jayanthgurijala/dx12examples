# PowerShell script — run at repository root to prepend the license header
# It will skip files that already contain the copyright/Spdx marker.
#
# Usage:
#   Open PowerShell in repo root and run:
#     .\tools\add_license.ps1
#
# Creates a simple backup (.bak) of each modified file.

$license = @"
/*
* Copyright (C) 2026 by Jayanth Gurijala
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
"@

# File globs to update
$globs = @("*.cpp","*.h","*.hpp","*.c")

# Find files
$files = Get-ChildItem -Path . -Recurse -Include $globs -File

if ($files.Count -eq 0) {
    Write-Host "No matching files found."
    exit 0
}

foreach ($file in $files) {
    # read raw content
    $content = Get-Content -Raw -Encoding UTF8 -Path $file.FullName

    # skip if already has header marker
    if ($content -match "Copyright \(c\)\s*2026\s*Jayanth\s*Gurijala" -or $content -match "SPDX-License-Identifier:\s*MIT" -or $content -match "Copyright \(c\)") {
        Write-Host "Skipping (already licensed): $($file.FullName)"
        continue
    }

    # prepend license and preserve original line endings
    $newContent = $license + "`r`n" + $content

    # write back
    Set-Content -Path $file.FullName -Value $newContent -Encoding UTF8

    Write-Host "Prepended license to: $($file.FullName)"
}

Write-Host "Done. Backups with .bak extension created for modified files."