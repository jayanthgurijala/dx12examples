#pragma once

#include "dxtypes.h"
#include <DirectXMath.h>

using namespace DirectX;

namespace DxTransformHelper
{
    inline void QuaternionIdentityRotation(DxTransformInfo& transformInfo)
    {
        transformInfo.trsInfo.rotationMode = DxRotationModeQuaternion;
        transformInfo.trsInfo.quaterion.x = 0;
        transformInfo.trsInfo.quaterion.y = 0;
        transformInfo.trsInfo.quaterion.z = 0;
        transformInfo.trsInfo.quaterion.w = 1;
    }

    inline void SetQuaternionValues(DxTransformInfo& transformInfo, std::vector<double> rotation)
    {
        transformInfo.trsInfo.rotationMode = DxRotationModeQuaternion;
        transformInfo.trsInfo.quaterion.x = static_cast<FLOAT>(rotation[0]);
        transformInfo.trsInfo.quaterion.y = static_cast<FLOAT>(rotation[1]);
        transformInfo.trsInfo.quaterion.z = static_cast<FLOAT>(rotation[2]);
        transformInfo.trsInfo.quaterion.w = static_cast<FLOAT>(rotation[3]);
    }

    inline void SetQuaternionRotation(DxTransformInfo& transformInfo, std::vector<double> rotation, BOOL hasRotation)
    {
        if (hasRotation)
        {
            SetQuaternionValues(transformInfo, rotation);
        }
        else
        {
            QuaternionIdentityRotation(transformInfo);
        }
    }

    inline void TranslationIdentity(DxTransformInfo& transformInfo)
    {
        transformInfo.trsInfo.translation[0] = transformInfo.trsInfo.translation[1] = transformInfo.trsInfo.translation[2] = 0;
    }

    inline void SetTranslationValues(DxTransformInfo& transformInfo, std::vector<double> translation)
    {
        transformInfo.trsInfo.translation[0] = static_cast<FLOAT>(translation[0]);
        transformInfo.trsInfo.translation[1] = static_cast<FLOAT>(translation[1]);
        transformInfo.trsInfo.translation[2] = static_cast<FLOAT>(translation[2]);
    }

    inline void SetTranslation(DxTransformInfo& transformInfo, std::vector<double> translation, BOOL hasTranslation)
    {
        if (hasTranslation)
        {
            SetTranslationValues(transformInfo, translation);
        }
        else
        {
            TranslationIdentity(transformInfo);
        }
    }

    inline void ScaleIdentity(DxTransformInfo& transformInfo)
    {
        transformInfo.trsInfo.scale[0] = transformInfo.trsInfo.scale[1] = transformInfo.trsInfo.scale[2] = 0;
    }

    inline void SetScaleValues(DxTransformInfo& transformInfo, std::vector<double> scale)
    {
        transformInfo.trsInfo.scale[0] = static_cast<FLOAT>(scale[0]);
        transformInfo.trsInfo.scale[1] = static_cast<FLOAT>(scale[1]);
        transformInfo.trsInfo.scale[2] = static_cast<FLOAT>(scale[2]);
    }

    inline void SetScale(DxTransformInfo& transformInfo, std::vector<double> scale, BOOL hasScale)
    {
        if (hasScale)
        {
            SetScaleValues(transformInfo, scale);
        }
        else
        {
            ScaleIdentity(transformInfo);
        }
    }

    inline void SetTranslationValues(DxTransformInfo& transformInfo, float x, float y, float z)
    {
        transformInfo.trsInfo.translation[0] = x;
        transformInfo.trsInfo.translation[1] = y;
        transformInfo.trsInfo.translation[2] = z;
    }

    inline void SetTranslationValues(DxTransformInfo& transformInfo, const FLOAT values[])
    {
        SetTranslationValues(transformInfo, values[0], values[1], values[2]);
    }

    inline void SetTranslationIdentityValues(DxTransformInfo& transformInfo)
    {
        SetTranslationValues(transformInfo, 0, 0, 0);
    }

    
    inline void SetScaleValues(DxTransformInfo& transformInfo, float x, float y, float z)
    {
        transformInfo.trsInfo.scale[0] = x;
        transformInfo.trsInfo.scale[1] = y;
        transformInfo.trsInfo.scale[2] = z;
    }

    inline void SetScaleValues(DxTransformInfo& transformInfo, const FLOAT values[])
    {
        SetScaleValues(transformInfo, values[0], values[1], values[2]);
    }

    inline void SetScaleIdentityValues(DxTransformInfo& transformInfo)
    {
        SetScaleValues(transformInfo, 1.0f, 1.0f, 1.0f);
    }

    inline void SetRotationInDegrees(DxTransformInfo& transformInfo, float x, float y, float z)
    {
        assert(transformInfo.trsInfo.rotationMode == DxRotationModeInvalid);
        transformInfo.trsInfo.rotationMode = DxRotationModeDegree;
        transformInfo.trsInfo.degree.xDeg = x;
        transformInfo.trsInfo.degree.yDeg = y;
        transformInfo.trsInfo.degree.zDeg = z;
    }
   

    inline void SetRotationInDegrees(DxTransformInfo& transformInfo, const FLOAT values[])
    {
        SetRotationInDegrees(transformInfo, values[0], values[1], values[2]);
    }

    inline void SetRotationIdentityDegrees(DxTransformInfo& transformInfo)
    {
        SetRotationInDegrees(transformInfo, 0, 0, 0);
   }


    inline XMMATRIX GetXMTranslation(const DxTransformInfo& transformInfo)
    {
        return  XMMatrixTranslation(
            transformInfo.trsInfo.translation[0],
            transformInfo.trsInfo.translation[1],
            transformInfo.trsInfo.translation[2]
        );
    }

    inline XMMATRIX GetXMScale(const DxTransformInfo& transformInfo)
    {
        return XMMatrixScaling(
            transformInfo.trsInfo.scale[0],
            transformInfo.trsInfo.scale[1],
            transformInfo.trsInfo.scale[2]
        );
    }

    inline XMMATRIX GetXMRotationQuaternion(const DxTransformInfo& transformInfo)
    {
        assert(transformInfo.trsInfo.rotationMode == DxRotationModeQuaternion);
        XMVECTOR q = XMVectorSet(
            transformInfo.trsInfo.quaterion.x,
            transformInfo.trsInfo.quaterion.y,
            transformInfo.trsInfo.quaterion.z,
            transformInfo.trsInfo.quaterion.w
            );

        return XMMatrixRotationQuaternion(q);
    }

    inline XMMATRIX GetXMRotationDegree(const DxTransformInfo& transformInfo)
    {
        assert(transformInfo.trsInfo.rotationMode == DxRotationModeDegree);
        XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(transformInfo.trsInfo.degree.xDeg),
            XMConvertToRadians(transformInfo.trsInfo.degree.yDeg),
            XMConvertToRadians(transformInfo.trsInfo.degree.zDeg)
        );
        return rotMat;
    }

    inline XMMATRIX GetXMRotation(const DxTransformInfo& transformInfo)
    {
        auto rotationMode = transformInfo.trsInfo.rotationMode;
        XMMATRIX rotationMatrix;
        if (rotationMode == DxRotationModeQuaternion)
        {
            rotationMatrix = GetXMRotationQuaternion(transformInfo);
        }
        else if (rotationMode == DxRotationModeDegree)
        {
            rotationMatrix = GetXMRotationDegree(transformInfo);
        }
        else
        {
            assert(0);
        }
        return rotationMatrix;
    }

    inline XMMATRIX GetWorldMatrix(const DxTransformInfo& transformInfo)
    {
        auto T = GetXMTranslation(transformInfo);
        auto S = GetXMScale(transformInfo);
        auto R = GetXMRotation(transformInfo);

        XMMATRIX worldMatrix = S * R * T;

        return worldMatrix;

    }

    inline XMMATRIX GetCombinedWorldMatrix(const DxTransformInfo& worldTransform, const DxTransformInfo& transformToApply)
    {
        XMMATRIX worldTransformMatrix   = GetWorldMatrix(worldTransform);
        XMMATRIX transformToApplyMatrix = GetWorldMatrix(transformToApply);
        return worldTransformMatrix* transformToApplyMatrix;
    }

    inline XMFLOAT4X4 GetWorldMatrixDataTransposed(const DxTransformInfo& transformInfo)
    {
        auto worldMatrix = GetWorldMatrix(transformInfo);
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, XMMatrixTranspose(worldMatrix));
        return matrixData;
    }

    inline XMFLOAT4X4 GetWorldMatrixDataTransposed(XMMATRIX transformMatrix)
    {
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, XMMatrixTranspose(transformMatrix));
        return matrixData;
    }

    inline XMFLOAT4X4 GetCombinedWorldMatrixData(const DxTransformInfo& worldTransform, const DxTransformInfo& transformToApply)
    {
        return GetWorldMatrixDataTransposed(GetCombinedWorldMatrix(worldTransform, transformToApply));
    }


    inline XMFLOAT4X4 GetCombinedWorldMatrixData(const DxTransformInfo& worldTransform, const XMMATRIX& transformToApply)
    {
        return GetWorldMatrixDataTransposed(GetWorldMatrix(worldTransform) * transformToApply);
    }


};

