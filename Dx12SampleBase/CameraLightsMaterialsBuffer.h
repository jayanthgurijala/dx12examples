#pragma once



#include "dxtypes.h"


class CameraLightsMaterialsBuffer
{

    ///@note Data in the buffer has different frequency of change
    ///Ordered from least to highest frequency -
    ///Projection Matrix - almost never changes    (lowest change frequency)
    ///Lights - only changes if moving else constant
    ///View matrix -  same as Lights
    ///Materials - changes per primitive
    ///Model matrix - changes per instance (highest change frequency)
    /// 
    /// 
    /// Buffer Layout [ Scene Data | Per Instance Geometry Data | Material Data]
    /// 
    /// Scene Data (just one value for each which might change e.g. user interaction)
    ///          values: camera position, FovY, Inverse_viewProj(RayTracing)
    ///          data: from DxCamera because user interaction changes this
    /// 
    ///  Per Instance Geometry Data (a ton of values, each matrix has different values per instance 
    /// e.g. OakTree has 2 nodes, if scene has 10 instances, we need 20 below matrices
    ///               values: model matrix, normal matrix, MVP Matrix, 
    ///               data: gltf trs matrix + instance trs from scene description combines with view-projection above
    ///               [data for
    /// 
    /// Material Data (per primitive)
    /// e.g OakTree has 2 primitives and each primitive has one MaterialCB so total of 2.
    /// Instances of OakTree share MaterialData
    ///                values: MaterialCB
    ///                data: loaded from gltf and some flags
    /// 
    /// Class responsibility:
    /// - Calculate total buffer size 
    /// - allocate buffer
    /// - fill in data
    /// - return GPUVA for each chunk i.e scene, geometry, material
    /// - 
    ///           
    
public:
    CameraLightsMaterialsBuffer();

protected:
private:
    ComPtr<ID3D12Resource> bufferResource;

};

