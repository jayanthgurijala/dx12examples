cd /d "%~dp0"

del *.cso *.pdb 2> nul

set SHADERNAME=SimplePBR.hlsl

dxc -T vs_6_0 -E VSMain -Fo FrameSimple_VS.cso -Zi -Od -Qembed_debug -Fd FrameSimple_VS.pdb FrameSimple.hlsl
dxc -T ps_6_0 -E PSMain -Fo FrameSimple_PS.cso -Zi -Od -Qembed_debug -Fd FrameSimple_PS.pdb FrameSimple.hlsl
dxc -T cs_6_0 -E GenerateMipCS -Fo CsGenerateMips.cso -Zi -Od -Qembed_debug -Fd CsGenerateMips.pdb CsGenerateMips.hlsl

dxc -T vs_6_0 -E VSMain_1 -Fo Simple1_VS.cso -Zi -Od -Qembed_debug -Fd Simple1_VS.pdb %SHADERNAME%
dxc -T vs_6_0 -E VSMain_2 -Fo Simple2_VS.cso -Zi -Od -Qembed_debug -Fd Simple2_VS.pdb %SHADERNAME%
dxc -T vs_6_0 -E VSMain_3 -Fo Simple3_VS.cso -Zi -Od -Qembed_debug -Fd Simple3_VS.pdb %SHADERNAME%
dxc -T vs_6_0 -E VSMain_4 -Fo Simple4_VS.cso  -Zi -Od -Qembed_debug -Fd Simple4_VS.pdb %SHADERNAME%
dxc -T ps_6_0 -E PSMain -Fo SimplePS.cso  -Zi -Od -Qembed_debug -Fd SimplePS.pdb %SHADERNAME%

dxc -T hs_6_0 -E HSMain -Fo TessPassthrough_HS.cso %SHADERNAME%

dxc -T ds_6_0 -E DSMain_Pass -Fo TessPassthrough_DS.cso %SHADERNAME%
dxc -T ds_6_0 -E DSMain_PN -Fo TessFactor3_DS.cso %SHADERNAME%



dxc /T lib_6_6 /Fo RaytraceSimpleCHS.cso -Zi -Od -Qembed_debug -Fd RaytraceSimpleCHS.pdb RaytraceSimpleCHS.hlsl

dxc /T ms_6_6 -E MSMain -Fo HelloMesh_MS.cso -Zi -Od -Qembed_debug -Fd HelloMesh_MS.pdb HelloMesh.hlsl
dxc /T ps_6_6 -E PSMain -Fo HelloMesh_PS.cso -Zi -Od -Qembed_debug -Fd HelloMesh_PS.pdb HelloMesh.hlsl






