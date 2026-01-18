cd /d "%~dp0"

del *.cso

for %%f in (*.hlsl) do (
    echo "================ compiling %%f =============="
    fxc /T vs_5_0 /E VSMain /Fo "%%~nf_VS.cso" "%%f"
    fxc /T ps_5_0 /E PSMain /Fo "%%~nf_PS.cso" "%%f"
)


REM fxc /T vs_5_0 /E VSMain /Fo  Simple1_VS.cso Simple1_VS_PS.hlsl
REM fxc /T ps_5_0 /E PSMain /Fo  Simple1_PS.cso Simple1_VS_PS.hlsl