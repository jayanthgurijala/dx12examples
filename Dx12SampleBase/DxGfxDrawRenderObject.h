#pragma once

#include <d3d12.h>
#include "Dx12SampleBase.h"

class DxGfxDrawRenderObject
{
    
    /*   Has all the state to render a scene from a camera (main, secondary, lights etc) - need to have multiple viewProj matrices
     *   After setting all the state, it just calls RenderScene (using cpp interfaces ? )
     *   lights need not render to rtv but useful for debugging.
     *   So RenderScene would need to set all the state except RTV and DSV.
     *   value in adding this class is it can encapsulate all the state - RTV, DSV, RootSignature, PipelineState, Shaders
     *   even the main camera can use this class for the final rendering
     *   So how to split the logic?
     *   E.g In HelloForest
     *     Common? OnInit() -> 
                   RootSignature -> RenderObject will contribute to it.
                                 -> 0)viewProj Root CBV
                                 -> 1)Root Arg -> worldMatrix Root CBV
                                 -> 2)DescriptorTable -> SceneElement SRVs
                                 -> 3)Root Arg-?Material Properties Root CBV
                                 -> 4)App specific SRVs ?
                   PipelineState
     *     
     *      RenderFrame() ->
     * 
     * -------------- controlled by render object -----------------------------
     *          ClearRTV/DSV
     *          SetRTVDSV
     *          SetTopology
     *          SetGfxRootSig, SetGfxPiprlineState, SetDescriptorHeaps
     * 
     *          SetRootArgs -> ViewProj, Render Specific SRVs if any
     *          
     * -----------------------------------------------------------------------
     * 
     *          ForEachPrimitive()
     *            PipelineState -----> Might need multiple versions? maybe not
     *            SetRootArgs -> WorldMatrix, SRVs, Material Props
     *            Render
     * 
     * Summary: App creates various render objects to render intermediate and final content
     * - Render object needs (meaning contributes to count allocated by sample base)
     *       - a view proj matrix 
     *       - zero or more RTVs/DSVs
     *       - Zero or more SRVs produced by previous stage
     * 
     * - Render object produces
     *      - one more or SRVs
     *      - Reside in SRV descriptor heap
     */

public:
    DxGfxDrawRenderObject(UINT viewProjOffset, UINT numRtvs, UINT rtvHeapStartOffset, BOOL needsDsv, UINT dsvHeapStartOffset);
    inline UINT NumRTVs()
    {
        return m_numRtvs;
    }

    inline UINT NumDSVs()
    {
        return (m_needsDsv == TRUE) ? 1 : 0;
    }

    VOID RenderInitViewProjRtvDsv();
    VOID Render();

protected:
private:
    Dx12SampleBase* m_sampleBase;
    UINT m_numRtvs = 0;
    UINT m_rtvHeapStartOffset = 0;

    BOOL m_needsDsv = FALSE;
    UINT m_dsvHeapStartOffset = 0;

    UINT m_viewProjOffset;
};

