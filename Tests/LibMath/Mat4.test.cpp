/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <LibMath/Math.h>

TEST(Mat4, Construction)
{
    auto mat = Math::Mat4f::identity();
    EXPECT_EQ(mat[0], 1);
    EXPECT_EQ(mat[5], 1);
    EXPECT_EQ(mat[10], 1);
    EXPECT_EQ(mat[15], 1);

    Math::Mat4f mat2({
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    });
    for (size_t i = 0; i < 16; i++) {
        EXPECT_EQ(mat2[i], i + 1);
    }
}

TEST(Mat4, Translation)
{
    auto mat = Math::Mat4f::identity();
    auto translated = mat.translate({ 1.0F, 2.0F, 3.0F });
    EXPECT_EQ(translated[12], 1.0F);
    EXPECT_EQ(translated[13], 2.0F);
    EXPECT_EQ(translated[14], 3.0F);
}

TEST(Mat4, Scaling)
{
    auto mat = Math::Mat4f::scale({ 2.0F, 3.0F, 4.0F });
    EXPECT_EQ(mat[0], 2.0F);
    EXPECT_EQ(mat[5], 3.0F);
    EXPECT_EQ(mat[10], 4.0F);
}

TEST(Mat4, Multiply)
{
    auto m1 = Math::Mat4f({
        1, 5, 9, 13,
        2, 6, 10, 14,
        3, 7, 11, 15,
        4, 8, 12, 16
    });
    auto m2 = Math::Mat4f({
        16, 12, 8, 4,
        15, 11, 7, 3,
        14, 10, 6, 2,
        13, 9, 5, 1
    });
    auto actual = m1 * m2;
    auto expected = Math::Mat4f({
        80, 240, 400, 560,
        70, 214, 358, 502,
        60, 188, 316, 444,
        50, 162, 274, 386
    });
    EXPECT_EQ(actual.elements(), expected.elements());
}

TEST(Mat4, PerspectiveSymmetry)
{
    auto aspect = 1.0F;
    auto near = 0.1F;
    auto far = 10.0F;

    auto projection = Math::Mat4f::perspective(DEG_TO_RAD(90.0F), aspect, near, far);

    EXPECT_EQ(projection[1], 0.0F);
    EXPECT_EQ(projection[2], 0.0F);
    EXPECT_EQ(projection[4], 0.0F);
    EXPECT_EQ(projection[6], 0.0F);
}

TEST(Mat4, PerspectiveFovScaling)
{
    auto const aspect = 1.0F;
    auto const near = 0.1F;
    auto const far = 100.0F;

    auto const m1 = Math::Mat4f::perspective(DEG_TO_RAD(45.0F), aspect, near, far);
    auto const m2 = Math::Mat4f::perspective(DEG_TO_RAD(90.0F), aspect, near, far);

    EXPECT_GT(m1[0], m2[0]);
}

TEST(Mat4, PerspectiveDepthMapping)
{
    auto const aspect = 1.0F;
    auto const near = 0.1F;
    auto const far = 100.0F;

    auto const projection = Math::Mat4f::perspective(DEG_TO_RAD(60.0F), aspect, near, far);

    auto const on_near = projection * Math::Vec4f({ 0.0F, 0.0F, -near }, 1.0F);
    auto const on_far = projection * Math::Vec4f({ 0.0F, 0.0F, -far }, 1.0F);

    EXPECT_GT(on_near.w, 0.0F);
    EXPECT_GT(on_far.w, 0.0F);
    EXPECT_NEAR(on_near.z / on_near.w, 0.0F, 1e-5F);
    EXPECT_NEAR(on_far.z / on_far.w, 1.0F, 1e-5F);
}

TEST(Mat4, PerspectiveHandedness)
{
    auto const projection = Math::Mat4f::perspective(DEG_TO_RAD(90.0F), 1.0F, 0.1F, 100.0F);

    auto const right_edge = projection * Math::Vec4f({ 1.0F, 0.0F, -1.0F }, 1.0F);
    EXPECT_NEAR(right_edge.x / right_edge.w, 1.0F, 1e-5F);

    auto const top_edge = projection * Math::Vec4f({ 0.0F, 1.0F, -1.0F }, 1.0F);
    EXPECT_NEAR(top_edge.y / top_edge.w, -1.0F, 1e-5F);

    auto const behind = projection * Math::Vec4f({ 0.0F, 0.0F, 1.0F }, 1.0F);
    EXPECT_LT(behind.w, 0.0F);
}

TEST(Mat4, OrthographicDepthMapping)
{
    auto const near_plane = 0.1F;
    auto const far_plane = 100.0F;
    auto const actual = Math::Mat4f::orthographic(-10.0F, 10.0F, -10.0F, 10.0F, near_plane, far_plane);

    auto const on_near = actual * Math::Vec4f({ 0.0F, 0.0F, -near_plane }, 1.0F);
    auto const on_far = actual * Math::Vec4f({ 0.0F, 0.0F, -far_plane }, 1.0F);

    auto const eps = 1e-4F;
    EXPECT_NEAR(on_near.w, 1.0F, eps);
    EXPECT_NEAR(on_near.z, 0.0F, eps);
    EXPECT_NEAR(on_far.z, 1.0F, eps);
}

TEST(Mat4, OrthographicOffCenter)
{
    auto const actual = Math::Mat4f::orthographic(0.0F, 20.0F, 0.0F, 10.0F, 0.1F, 100.0F);

    auto const center = actual * Math::Vec4f({ 10.0F, 5.0F, -1.0F }, 1.0F);
    EXPECT_NEAR(center.x, 0.0F, 1e-5F);
    EXPECT_NEAR(center.y, 0.0F, 1e-5F);

    auto const corner = actual * Math::Vec4f({ 20.0F, 10.0F, -1.0F }, 1.0F);
    EXPECT_NEAR(corner.x, 1.0F, 1e-5F);
    EXPECT_NEAR(corner.y, -1.0F, 1e-5F);
}

TEST(Mat4, LookAtNegativeZ)
{
    auto const actual = Math::Mat4f::look_at({ 0.0F, 0.0F, 5.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F });
    auto const expected = Math::Mat4f({
        1.0F, 0.0F,  0.0F, 0.0F,
        0.0F, 1.0F,  0.0F, 0.0F,
        0.0F, 0.0F,  1.0F, 0.0F,
        0.0F, 0.0F, -5.0F, 1.0F
    });
    EXPECT_EQ(actual.elements(), expected.elements());
}

TEST(Mat4, LookAtNegativeX)
{
    auto const actual = Math::Mat4f::look_at({ 3.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F });
    auto const expected = Math::Mat4f({
         0.0F, 0.0F,  1.0F, 0.0F,
         0.0F, 1.0F,  0.0F, 0.0F,
        -1.0F, 0.0F,  0.0F, 0.0F,
         0.0F, 0.0F, -3.0F, 1.0F
    });
    EXPECT_EQ(actual.elements(), expected.elements());
}

TEST(Mat4, LookAtMapsEyeAndTarget)
{
    Math::Vec3f const eye { 4.0F, 3.0F, -2.0F };
    Math::Vec3f const target { -1.0F, 1.0F, 5.0F };
    auto const view = Math::Mat4f::look_at(eye, target, { 0.0F, 1.0F, 0.0F });

    auto const eye_in_view = view * Math::Vec4f(eye, 1.0F);
    EXPECT_NEAR(eye_in_view.x, 0.0F, 1e-4F);
    EXPECT_NEAR(eye_in_view.y, 0.0F, 1e-4F);
    EXPECT_NEAR(eye_in_view.z, 0.0F, 1e-4F);

    auto const target_in_view = view * Math::Vec4f(target, 1.0F);
    EXPECT_NEAR(target_in_view.x, 0.0F, 1e-4F);
    EXPECT_NEAR(target_in_view.y, 0.0F, 1e-4F);
    EXPECT_NEAR(target_in_view.z, -(target - eye).length(), 1e-3F);
}

TEST(Mat4, LookAtPreservesHandedness)
{
    auto const view = Math::Mat4f::look_at({ 3.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F });

    auto const on_positive_z = view * Math::Vec4f({ 0.0F, 0.0F, 1.0F }, 1.0F);
    EXPECT_NEAR(on_positive_z.x, -1.0F, 1e-5F);

    auto const above = view * Math::Vec4f({ 0.0F, 1.0F, 0.0F }, 1.0F);
    EXPECT_NEAR(above.y, 1.0F, 1e-5F);
}

TEST(Mat4, FromQuaternionIdentity)
{
    auto quat = Math::Quatf::identity();
    auto mat = Math::Mat4f::from_quaternion(quat);

    EXPECT_EQ(mat.elements(), Math::Mat4f::identity().elements());
}

TEST(Mat4, FromQuaternionRotatesLikeTheQuaternion)
{
    auto const quat = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(90.0F));
    auto const mat = Math::Mat4f::from_quaternion(quat);

    auto const rotated = mat * Math::Vec4f({ 1.0F, 0.0F, 0.0F }, 1.0F);
    EXPECT_NEAR(rotated.x, 0.0F, 1e-6F);
    EXPECT_NEAR(rotated.y, 1.0F, 1e-6F);
    EXPECT_NEAR(rotated.z, 0.0F, 1e-6F);

    auto const by_quat = quat * Math::Vec3f(0.3F, -0.7F, 0.5F);
    auto const by_mat = mat * Math::Vec4f({ 0.3F, -0.7F, 0.5F }, 1.0F);
    EXPECT_NEAR(by_mat.x, by_quat.x, 1e-6F);
    EXPECT_NEAR(by_mat.y, by_quat.y, 1e-6F);
    EXPECT_NEAR(by_mat.z, by_quat.z, 1e-6F);
}

TEST(Mat4, TranslateMatchesMultiplication)
{
    auto const projection = Math::Mat4f::perspective(DEG_TO_RAD(60.0F), 1.5F, 0.1F, 100.0F);
    Math::Vec3f const offset { 1.0F, -2.0F, 3.0F };

    auto const translated = projection.translate(offset);
    auto const multiplied = projection * Math::Mat4f::translation(offset);

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_NEAR(translated[i], multiplied[i], 1e-5F) << "element " << i;
    }
}
