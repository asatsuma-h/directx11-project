#pragma once
#include <DirectXMath.h>
#include <algorithm>
using namespace DirectX;

class Camera
{
public:
    Camera()
    {
        mPosition = { 0.0f, 0.0f, -3.0f };
        mTarget = { 0.0f, 0.0f,  0.0f };
        mUp = { 0.0f, 1.0f,  0.0f };
        mYaw = 0.0f;
        mPitch = 0.0f;
    }

    void Move(float dx, float dy, float dz)
    {
        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&mTarget), pos));
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&mUp), forward));

        pos += dx * right + dy * XMVectorSet(0, 1, 0, 0) + dz * forward;
        XMStoreFloat3(&mPosition, pos);
        XMStoreFloat3(&mTarget, pos + forward);
    }

    void Rotate(float yawDelta, float pitchDelta)
    {
        mYaw += yawDelta;
        mPitch = std::clamp(mPitch + pitchDelta, -XM_PIDIV2 * 0.99f, XM_PIDIV2 * 0.99f);

        XMMATRIX rot = XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0);
        XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
        XMVECTOR pos = XMLoadFloat3(&mPosition);
        XMStoreFloat3(&mTarget, pos + forward);
    }

    XMMATRIX GetViewMatrix() const
    {
        return XMMatrixLookAtLH(
            XMLoadFloat3(&mPosition),
            XMLoadFloat3(&mTarget),
            XMLoadFloat3(&mUp));
    }

    XMFLOAT3 GetPosition() { return mPosition; }

private:
    XMFLOAT3 mPosition;
    XMFLOAT3 mTarget;
    XMFLOAT3 mUp;
    float mYaw, mPitch;
};
