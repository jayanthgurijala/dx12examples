cd /d "%~dp0"

del *.cso

dxc -T vs_6_0 -E VSMain -Fo FrameSimple_VS.cso FrameSimple.hlsl
dxc -T ps_6_0 -E PSMain -Fo FrameSimple_PS.cso FrameSimple.hlsl

dxc -T vs_6_0 -E VSMain -Fo Simple1_VS.cso Simple1.hlsl
dxc -T ps_6_0 -E PSMain -Fo Simple1_PS.cso Simple1.hlsl

dxc -T vs_6_0 -E VSMain -Fo Simple5_VS.cso Simple5.hlsl
dxc -T ps_6_0 -E PSMain -Fo Simple5_PS.cso Simple5.hlsl

dxc -T vs_6_0 -E VSMain -Fo TessPassthrough_VS.cso TessPassthrough.hlsl
dxc -T hs_6_0 -E HSMain -Fo TessPassthrough_HS.cso TessPassthrough.hlsl
dxc -T ds_6_0 -E DSMain -Fo TessPassthrough_DS.cso TessPassthrough.hlsl
dxc -T ps_6_0 -E PSMain -Fo TessPassthrough_PS.cso TessPassthrough.hlsl

dxc -T vs_6_0 -E VSMain -Fo TessFactor_VS.cso TessFactor.hlsl
dxc -T hs_6_0 -E HSMain -Fo TessFactor_HS.cso TessFactor.hlsl
dxc -T ds_6_0 -E DSMain -Fo TessFactor_DS.cso TessFactor.hlsl
dxc -T ps_6_0 -E PSMain -Fo TessFactor_PS.cso TessFactor.hlsl

copy *.cso ..\x64\Debug\	




