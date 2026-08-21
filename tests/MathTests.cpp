// ---------------------------------------------------------------------------
// Tests for the portable DirectXMath implementation in compat/win32.
//
// These are not "does it compile" smoke tests. The engine's geometry depends on
// this header reproducing DirectXMath's *conventions* - row-vector transforms,
// left-handed projections, and a quaternion product whose argument order is the
// reverse of the usual Hamilton one. Getting any of those subtly wrong produces
// a renderer that looks almost right, which is the hardest kind of bug to find.
// So the tests below assert the conventions directly.
//
// On Windows this file compiles against the real DirectXMath from the SDK,
// which makes it a conformance test of the portable version against the
// original rather than a test of the portable version against itself.
// ---------------------------------------------------------------------------

#include "TestFramework.h"

#include <DirectXMath.h>

using namespace DirectX;

namespace {

constexpr float kEps = 1e-4f;

void CheckMatrixNear(const XMMATRIX& a, const XMMATRIX& b, float eps = kEps) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            XMFLOAT4X4 fa;
            XMFLOAT4X4 fb;
            XMStoreFloat4x4(&fa, a);
            XMStoreFloat4x4(&fb, b);
            CHECK_NEAR(fa.m[row][col], fb.m[row][col], eps);
        }
    }
}

// --- Vector basics ---------------------------------------------------------

NEXUS_TEST(VectorLoadStoreRoundTrips) {
    const XMFLOAT3 source(1.5f, -2.25f, 3.75f);
    XMFLOAT3 result;
    XMStoreFloat3(&result, XMLoadFloat3(&source));

    CHECK_NEAR(result.x, 1.5f, kEps);
    CHECK_NEAR(result.y, -2.25f, kEps);
    CHECK_NEAR(result.z, 3.75f, kEps);
}

NEXUS_TEST(LoadFloat3ZeroesTheWLane) {
    // XMVector3Dot relies on the w lane being zero rather than left over from a
    // previous value; a non-zero w here silently corrupts every dot product.
    const XMFLOAT3 v(1.0f, 2.0f, 3.0f);
    CHECK_NEAR(XMVectorGetW(XMLoadFloat3(&v)), 0.0f, kEps);
}

NEXUS_TEST(DotProductIgnoresW) {
    const XMVECTOR a = XMVectorSet(1.0f, 2.0f, 3.0f, 999.0f);
    const XMVECTOR b = XMVectorSet(4.0f, 5.0f, 6.0f, 999.0f);
    CHECK_NEAR(XMVectorGetX(XMVector3Dot(a, b)), 32.0f, kEps);
}

NEXUS_TEST(DotProductIsSplattedAcrossLanes) {
    // The SDK returns the scalar broadcast to all four lanes so the result can
    // be fed straight back into vector arithmetic.
    const XMVECTOR d = XMVector3Dot(XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f),
                                    XMVectorSet(4.0f, 5.0f, 6.0f, 0.0f));
    CHECK_NEAR(XMVectorGetX(d), 32.0f, kEps);
    CHECK_NEAR(XMVectorGetY(d), 32.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(d), 32.0f, kEps);
    CHECK_NEAR(XMVectorGetW(d), 32.0f, kEps);
}

NEXUS_TEST(CrossProductIsRightHandedOnAxes) {
    const XMVECTOR c = XMVector3Cross(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
                                      XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    CHECK_NEAR(XMVectorGetX(c), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetY(c), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(c), 1.0f, kEps);
}

NEXUS_TEST(NormalizeProducesUnitLength) {
    const XMVECTOR n = XMVector3Normalize(XMVectorSet(3.0f, -4.0f, 12.0f, 0.0f));
    CHECK_NEAR(XMVectorGetX(XMVector3Length(n)), 1.0f, kEps);
}

NEXUS_TEST(NormalizeOfZeroVectorDoesNotProduceNaN) {
    // A degenerate direction is easy to produce from user data. Propagating NaN
    // from here would poison every transform downstream of it.
    const XMVECTOR n = XMVector3Normalize(XMVectorZero());
    CHECK(!std::isnan(XMVectorGetX(n)));
    CHECK(!std::isnan(XMVectorGetY(n)));
    CHECK(!std::isnan(XMVectorGetZ(n)));
}

NEXUS_TEST(LerpHitsBothEndpoints) {
    const XMVECTOR a = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    const XMVECTOR b = XMVectorSet(10.0f, 20.0f, 30.0f, 0.0f);

    CHECK_NEAR(XMVectorGetX(XMVectorLerp(a, b, 0.0f)), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetX(XMVectorLerp(a, b, 1.0f)), 10.0f, kEps);
    CHECK_NEAR(XMVectorGetY(XMVectorLerp(a, b, 0.5f)), 10.0f, kEps);
}

// --- Matrix conventions ----------------------------------------------------

NEXUS_TEST(IdentityLeavesPointsUnchanged) {
    const XMVECTOR p = XMVector3Transform(XMVectorSet(3.0f, -7.0f, 11.0f, 0.0f),
                                          XMMatrixIdentity());
    CHECK_NEAR(XMVectorGetX(p), 3.0f, kEps);
    CHECK_NEAR(XMVectorGetY(p), -7.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(p), 11.0f, kEps);
}

NEXUS_TEST(TranslationLivesInTheFourthRow) {
    // This is the row-vector convention. If translation ended up in the fourth
    // *column* instead, points would transform as if by the transpose.
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, XMMatrixTranslation(3.0f, 4.0f, 5.0f));

    CHECK_NEAR(m._41, 3.0f, kEps);
    CHECK_NEAR(m._42, 4.0f, kEps);
    CHECK_NEAR(m._43, 5.0f, kEps);
}

NEXUS_TEST(TranslationMovesPointsButNotNormals) {
    const XMMATRIX t = XMMatrixTranslation(3.0f, 4.0f, 5.0f);
    const XMVECTOR one = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);

    // A point picks up the translation (w = 1).
    CHECK_NEAR(XMVectorGetX(XMVector3Transform(one, t)), 4.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(XMVector3Transform(one, t)), 6.0f, kEps);

    // A direction must not (w = 0), or lighting breaks under translation.
    CHECK_NEAR(XMVectorGetX(XMVector3TransformNormal(one, t)), 1.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(XMVector3TransformNormal(one, t)), 1.0f, kEps);
}

NEXUS_TEST(RotationZTurnsXTowardsY) {
    const XMVECTOR v = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
                                                XMMatrixRotationZ(XM_PIDIV2));
    CHECK_NEAR(XMVectorGetX(v), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetY(v), 1.0f, kEps);
}

NEXUS_TEST(MultiplicationAppliesLeftOperandFirst) {
    // XMMatrixMultiply(A, B) must mean "A then B". Reversing this is the
    // classic row-vs-column-vector mistake and yields scaled translations.
    const XMMATRIX scaleThenTranslate =
        XMMatrixMultiply(XMMatrixScaling(2.0f, 2.0f, 2.0f),
                         XMMatrixTranslation(10.0f, 0.0f, 0.0f));

    const XMVECTOR p = XMVector3Transform(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
                                          scaleThenTranslate);

    // Scaled to 2, then translated by 10.
    CHECK_NEAR(XMVectorGetX(p), 12.0f, kEps);
}

NEXUS_TEST(RotationMatricesHaveUnitDeterminant) {
    const XMVECTOR axis = XMVector3Normalize(XMVectorSet(0.3f, 0.7f, -0.2f, 0.0f));
    const XMMATRIX r = XMMatrixRotationAxis(axis, 0.9f);
    CHECK_NEAR(XMVectorGetX(XMMatrixDeterminant(r)), 1.0f, kEps);
}

NEXUS_TEST(InverseUndoesTheOriginalTransform) {
    const XMMATRIX m(1.0f,  2.0f,  3.0f,  4.0f,
                     5.0f,  6.0f,  2.0f,  8.0f,
                     9.0f,  1.0f, 11.0f, 12.0f,
                     13.0f, 14.0f, 15.0f, 2.0f);

    XMVECTOR det;
    CheckMatrixNear(XMMatrixMultiply(m, XMMatrixInverse(&det, m)),
                    XMMatrixIdentity(), 1e-3f);
    CHECK(std::fabs(XMVectorGetX(det)) > 1e-6f);
}

NEXUS_TEST(InverseOfSingularMatrixStaysFinite) {
    // A zero-scale transform is easy to produce from a scene file. The result
    // is meaningless either way, but it must not be NaN.
    const XMMATRIX singular = XMMatrixScaling(0.0f, 0.0f, 0.0f);

    XMVECTOR det;
    XMFLOAT4X4 out;
    XMStoreFloat4x4(&out, XMMatrixInverse(&det, singular));

    CHECK_NEAR(XMVectorGetX(det), 0.0f, kEps);
    CHECK(!std::isnan(out._11));
}

NEXUS_TEST(TransposeSwapsRowsAndColumns) {
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, XMMatrixTranspose(XMMatrixTranslation(3.0f, 4.0f, 5.0f)));

    // The translation row becomes the translation column.
    CHECK_NEAR(m._14, 3.0f, kEps);
    CHECK_NEAR(m._24, 4.0f, kEps);
    CHECK_NEAR(m._34, 5.0f, kEps);
}

NEXUS_TEST(Float4x4RoundTripsThroughXMMATRIX) {
    const XMMATRIX m(1.0f,  2.0f,  3.0f,  4.0f,
                     5.0f,  6.0f,  7.0f,  8.0f,
                     9.0f,  10.0f, 11.0f, 12.0f,
                     13.0f, 14.0f, 15.0f, 16.0f);
    XMFLOAT4X4 stored;
    XMStoreFloat4x4(&stored, m);
    CheckMatrixNear(XMLoadFloat4x4(&stored), m);

    // Storage order must be row-major for constant-buffer uploads to be correct.
    CHECK_NEAR(stored._11, 1.0f, kEps);
    CHECK_NEAR(stored._12, 2.0f, kEps);
    CHECK_NEAR(stored._21, 5.0f, kEps);
}

// --- Camera and projection -------------------------------------------------

NEXUS_TEST(LookAtPlacesTheEyeAtTheOrigin) {
    const XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f),
                                           XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                                           XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    // The eye itself maps to the view-space origin...
    const XMVECTOR eye = XMVector3Transform(XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f), view);
    CHECK_NEAR(XMVectorGetX(eye), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetY(eye), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(eye), 0.0f, kEps);

    // ...and the focus point ends up straight ahead at +z (left-handed).
    const XMVECTOR focus = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), view);
    CHECK_NEAR(XMVectorGetZ(focus), 5.0f, kEps);
}

NEXUS_TEST(LookAtKeepsUpDirectionUpright) {
    const XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(4.0f, 3.0f, -2.0f, 0.0f),
                                           XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
                                           XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    // World up must still have a positive y component in view space, otherwise
    // the camera is upside down.
    const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), view);
    CHECK(XMVectorGetY(up) > 0.0f);
}

NEXUS_TEST(PerspectiveMapsNearToZeroAndFarToOne) {
    // Direct3D clip space is z in [0, 1], unlike OpenGL's [-1, 1]. Getting this
    // wrong halves the usable depth buffer.
    const float nearZ = 0.5f;
    const float farZ = 100.0f;
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, nearZ, farZ);

    const XMVECTOR atNear = XMVector4Transform(XMVectorSet(0.0f, 0.0f, nearZ, 1.0f), proj);
    const XMVECTOR atFar = XMVector4Transform(XMVectorSet(0.0f, 0.0f, farZ, 1.0f), proj);

    CHECK_NEAR(XMVectorGetZ(atNear) / XMVectorGetW(atNear), 0.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(atFar) / XMVectorGetW(atFar), 1.0f, kEps);
}

NEXUS_TEST(PerspectiveWComponentCarriesViewDepth) {
    // The perspective divide depends on w receiving z, not 1.
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.0f, 1.0f, 100.0f);
    const XMVECTOR p = XMVector4Transform(XMVectorSet(0.0f, 0.0f, 42.0f, 1.0f), proj);
    CHECK_NEAR(XMVectorGetW(p), 42.0f, kEps);
}

NEXUS_TEST(PerspectiveAppliesAspectRatioToX) {
    const XMMATRIX wide = XMMatrixPerspectiveFovLH(XM_PIDIV4, 2.0f, 1.0f, 100.0f);
    const XMMATRIX square = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.0f, 1.0f, 100.0f);

    XMFLOAT4X4 w;
    XMFLOAT4X4 s;
    XMStoreFloat4x4(&w, wide);
    XMStoreFloat4x4(&s, square);

    // A wider aspect ratio compresses x and leaves y alone.
    CHECK_NEAR(w._11, s._11 / 2.0f, kEps);
    CHECK_NEAR(w._22, s._22, kEps);
}

NEXUS_TEST(OrthographicMapsTheViewVolumeToClipSpace) {
    const XMMATRIX ortho = XMMatrixOrthographicLH(20.0f, 10.0f, 1.0f, 50.0f);

    const XMVECTOR corner = XMVector3Transform(XMVectorSet(10.0f, 5.0f, 1.0f, 0.0f), ortho);
    CHECK_NEAR(XMVectorGetX(corner), 1.0f, kEps);
    CHECK_NEAR(XMVectorGetY(corner), 1.0f, kEps);
    CHECK_NEAR(XMVectorGetZ(corner), 0.0f, kEps);

    const XMVECTOR farPlane = XMVector3Transform(XMVectorSet(0.0f, 0.0f, 50.0f, 0.0f), ortho);
    CHECK_NEAR(XMVectorGetZ(farPlane), 1.0f, kEps);
}

// --- Quaternions -----------------------------------------------------------

NEXUS_TEST(QuaternionAndAxisAngleAgree) {
    const XMVECTOR axis = XMVector3Normalize(XMVectorSet(0.3f, 0.7f, -0.2f, 0.0f));
    const float angle = 0.9f;

    CheckMatrixNear(XMMatrixRotationQuaternion(XMQuaternionRotationAxis(axis, angle)),
                    XMMatrixRotationAxis(axis, angle));
}

NEXUS_TEST(QuaternionMultiplyMatchesMatrixMultiplyOrder) {
    // DirectXMath's XMQuaternionMultiply(q1, q2) means "q1 then q2", which is
    // the reverse of the conventional Hamilton product. Animation blending
    // chains these, so an inverted order shows up as limbs rotating backwards.
    const XMVECTOR q1 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.4f);
    const XMVECTOR q2 = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.7f);

    CheckMatrixNear(XMMatrixRotationQuaternion(XMQuaternionMultiply(q1, q2)),
                    XMMatrixMultiply(XMMatrixRotationQuaternion(q1),
                                     XMMatrixRotationQuaternion(q2)));
}

NEXUS_TEST(SlerpReachesBothEndpoints) {
    const XMVECTOR q0 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.4f);
    const XMVECTOR q1 = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.7f);

    const XMVECTOR start = XMQuaternionSlerp(q0, q1, 0.0f);
    const XMVECTOR end = XMQuaternionSlerp(q0, q1, 1.0f);

    CHECK_NEAR(XMVectorGetX(start), XMVectorGetX(q0), kEps);
    CHECK_NEAR(XMVectorGetW(start), XMVectorGetW(q0), kEps);
    CHECK_NEAR(XMVectorGetX(end), XMVectorGetX(q1), kEps);
    CHECK_NEAR(XMVectorGetW(end), XMVectorGetW(q1), kEps);
}

NEXUS_TEST(SlerpStaysNormalizedThroughout) {
    // A drifting magnitude turns into creeping scale on skinned meshes.
    const XMVECTOR q0 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.4f);
    const XMVECTOR q1 = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 2.7f);

    for (int step = 0; step <= 10; ++step) {
        const float t = static_cast<float>(step) / 10.0f;
        CHECK_NEAR(XMVectorGetX(XMQuaternionLength(XMQuaternionSlerp(q0, q1, t))), 1.0f, kEps);
    }
}

NEXUS_TEST(SlerpTakesTheShortestPath) {
    // With a negative dot product the interpolation must flip one quaternion,
    // otherwise a small rotation is taken the long way around.
    const XMVECTOR q0 = XMQuaternionIdentity();
    const XMVECTOR q1 = XMVectorNegate(XMQuaternionIdentity());

    const XMVECTOR mid = XMQuaternionSlerp(q0, q1, 0.5f);

    // Both inputs represent no rotation, so the midpoint must too.
    CheckMatrixNear(XMMatrixRotationQuaternion(mid), XMMatrixIdentity());
}

NEXUS_TEST(SlerpHandlesNearlyIdenticalOrientations) {
    // sin(omega) underflows here; without the lerp fallback this divides by zero.
    const XMVECTOR q0 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.5f);
    const XMVECTOR q1 = XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), 0.5000001f);

    const XMVECTOR mid = XMQuaternionSlerp(q0, q1, 0.5f);
    CHECK(!std::isnan(XMVectorGetX(mid)));
    CHECK_NEAR(XMVectorGetX(XMQuaternionLength(mid)), 1.0f, kEps);
}

NEXUS_TEST(ConjugateInvertsAUnitQuaternion) {
    const XMVECTOR q = XMQuaternionRotationAxis(XMVector3Normalize(XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f)), 1.1f);
    CheckMatrixNear(XMMatrixRotationQuaternion(XMQuaternionMultiply(q, XMQuaternionConjugate(q))),
                    XMMatrixIdentity());
}

// --- Angle helpers ---------------------------------------------------------

NEXUS_TEST(DegreeRadianConversionRoundTrips) {
    CHECK_NEAR(XMConvertToRadians(180.0f), XM_PI, kEps);
    CHECK_NEAR(XMConvertToDegrees(XM_PI), 180.0f, kEps);
    CHECK_NEAR(XMConvertToDegrees(XMConvertToRadians(37.5f)), 37.5f, kEps);
}

} // namespace

int main() {
    RUN_TEST(VectorLoadStoreRoundTrips);
    RUN_TEST(LoadFloat3ZeroesTheWLane);
    RUN_TEST(DotProductIgnoresW);
    RUN_TEST(DotProductIsSplattedAcrossLanes);
    RUN_TEST(CrossProductIsRightHandedOnAxes);
    RUN_TEST(NormalizeProducesUnitLength);
    RUN_TEST(NormalizeOfZeroVectorDoesNotProduceNaN);
    RUN_TEST(LerpHitsBothEndpoints);

    RUN_TEST(IdentityLeavesPointsUnchanged);
    RUN_TEST(TranslationLivesInTheFourthRow);
    RUN_TEST(TranslationMovesPointsButNotNormals);
    RUN_TEST(RotationZTurnsXTowardsY);
    RUN_TEST(MultiplicationAppliesLeftOperandFirst);
    RUN_TEST(RotationMatricesHaveUnitDeterminant);
    RUN_TEST(InverseUndoesTheOriginalTransform);
    RUN_TEST(InverseOfSingularMatrixStaysFinite);
    RUN_TEST(TransposeSwapsRowsAndColumns);
    RUN_TEST(Float4x4RoundTripsThroughXMMATRIX);

    RUN_TEST(LookAtPlacesTheEyeAtTheOrigin);
    RUN_TEST(LookAtKeepsUpDirectionUpright);
    RUN_TEST(PerspectiveMapsNearToZeroAndFarToOne);
    RUN_TEST(PerspectiveWComponentCarriesViewDepth);
    RUN_TEST(PerspectiveAppliesAspectRatioToX);
    RUN_TEST(OrthographicMapsTheViewVolumeToClipSpace);

    RUN_TEST(QuaternionAndAxisAngleAgree);
    RUN_TEST(QuaternionMultiplyMatchesMatrixMultiplyOrder);
    RUN_TEST(SlerpReachesBothEndpoints);
    RUN_TEST(SlerpStaysNormalizedThroughout);
    RUN_TEST(SlerpTakesTheShortestPath);
    RUN_TEST(SlerpHandlesNearlyIdenticalOrientations);
    RUN_TEST(ConjugateInvertsAUnitQuaternion);

    RUN_TEST(DegreeRadianConversionRoundTrips);

    return NexusTest::Summarize("MathTests");
}
