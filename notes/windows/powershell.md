
Set-Item -path env:LIBCLANG_PATH -value                                                                             



Set-Item -path env:LIBCLANG_PATH -value "D:\Program Files\LLVM\bin"


Set-Alias git hub

cat $env:ALACRITTY_LOG

$env:path -split ';'
$newPath -join ';'

===
$oldPath = $env:Path -split ';'
$invalidPath = $oldPath | Where-Object {
-not(Test-Path ([environment]::ExpandEnvironmentVariables($_)))
}
if($invalidPath){
Write-Host "请选择你想删除的环境变量项！"
$selectedPath = $invalidPath | Out-GridView -Title "请选择你想删除的环境变量项！" -PassThru
if($selectedPath){
$newPath = $oldPath | Where-Object {
$selectedPath -notcontains $_
}

Write-Host '当前会话中的$env:Path已保存'
$env:Path = $newPath -join ';'

# 如果想将环境变量的更改保存进系统，请去掉下面一行的注释(需要管理员权限)
# [environment]::SetEnvironmentVariable('Path',$env:Path,'Machine')
}
}


---
$env:Path += ";SomeRandomPath"
$env:Path += ";I:\bin\nvim-win64\bin"
