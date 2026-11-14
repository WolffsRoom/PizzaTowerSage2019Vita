$dump = "C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\Logs\NewLog\psp2core-1794193747-0x00002f253b-eboot.bin.psp2dmp"
$bytes = [System.IO.File]::ReadAllBytes($dump)
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
$matches = [regex]::Matches($ascii, "[\x20-\x7E]{5,}")
$matches | Select-Object -First 50 | ForEach-Object { $_.Value }

