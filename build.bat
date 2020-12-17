@echo off
if exist "%VSVARS32_2008%" (
  call "%VSVARS32_2008%"
  if "%1" == "Clean" (
    echo -*-Clean-*-
    VCBuild /clean /nocolor "%2_VS2008.sln"
  )else if "%1" == "Rebuild" (
    echo -*-Rebuild-*-
    VCBuild /rebuild /nocolor "%2_VS2008.sln"
  )else if "%1" == "Build" (
    echo -*-Build-*-
    VCBuild /nocolor "%2_VS2008.sln"
  )else exit 1
)else if exist "%VSMSBUILDCMD%" (
  call "%VSMSBUILDCMD%"
  MSBuild /t:%1 /p:Configuration=Release /p:platform=Win32 "%2.sln" || exit /k 1
  MSBuild /t:%1 /p:Configuration=Release /p:platform=x64 "%2.sln"
)else exit 1
