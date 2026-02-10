cd /d "%~dp0"

del *.cso

dxc -T vs_6_0 -E VSMain -Fo FrameSimple_VS.cso -Zi -Od -Qembed_debug -Fd FrameSimple_VS.pdb FrameSimple.hlsl
dxc -T ps_6_0 -E PSMain -Fo FrameSimple_PS.cso -Zi -Od -Qembed_debug -Fd FrameSimple_PS.pdb FrameSimple.hlsl

dxc -T vs_6_0 -E VSMain -Fo Simple1_VS.cso -Zi -Od -Qembed_debug -Fd Simple1_VS.pdb Simple1.hlsl
dxc -T ps_6_0 -E PSMain -Fo Simple1_PS.cso -Zi -Od -Qembed_debug -Fd Simple1_PS.pdb Simple1.hlsl

dxc -T vs_6_0 -E VSMain -Fo Simple5_VS.cso  -Zi -Od -Qembed_debug -Fd Simple5_VS.pdb Simple5.hlsl
dxc -T ps_6_0 -E PSMain -Fo Simple5_PS.cso  -Zi -Od -Qembed_debug -Fd Simple5_PS.pdb Simple5.hlsl

dxc -T vs_6_0 -E VSMain -Fo TessPassthrough_VS.cso TessPassthrough.hlsl
dxc -T hs_6_0 -E HSMain -Fo TessPassthrough_HS.cso TessPassthrough.hlsl
dxc -T ds_6_0 -E DSMain -Fo TessPassthrough_DS.cso TessPassthrough.hlsl
dxc -T ps_6_0 -E PSMain -Fo TessPassthrough_PS.cso TessPassthrough.hlsl

dxc -T vs_6_0 -E VSMain -Fo TessFactor_VS.cso TessFactor.hlsl
dxc -T hs_6_0 -E HSMain -Fo TessFactor_HS.cso TessFactor.hlsl
dxc -T ds_6_0 -E DSMain -Fo TessFactor_DS.cso TessFactor.hlsl
dxc -T ps_6_0 -E PSMain -Fo TessFactor_PS.cso TessFactor.hlsl

dxc /T lib_6_6 /Fo RaytraceSimpleCHS.cso -Zi -Od -Qembed_debug -Fd RaytraceSimpleCHS.pdb RaytraceSimpleCHS.hlsl

copy *.cso ..\x64\Debug\
copy *.hlsl ..\x64\Debug\
copy *.cso ..\x64\Debug\
copy *.pdb ..\x64\Debug\





