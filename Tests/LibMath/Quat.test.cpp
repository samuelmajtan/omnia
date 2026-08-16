/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <LibMath/Math.h>

TEST(Quat, Construction)
{
    auto quat = Math::Quatf::identity();
    EXPECT_EQ(quat.x, 0);
    EXPECT_EQ(quat.y, 0);
    EXPECT_EQ(quat.z, 0);
    EXPECT_EQ(quat.w, 1);

    Math::Quatf quat2(1.0F, 2.0F, 3.0F, 4.0F);
    EXPECT_EQ(quat2.x, 1.0F);
    EXPECT_EQ(quat2.y, 2.0F);
    EXPECT_EQ(quat2.z, 3.0F);
    EXPECT_EQ(quat2.w, 4.0F);
}

TEST(Quat, Length)
{
    Math::Quatf quat(1.0F, 2.0F, 3.0F, 4.0F);
    auto expected = std::sqrt(1.0F + 4.0F + 9.0F + 16.0F);
    EXPECT_EQ(quat.length(), expected);
}

TEST(Quat, Normalize)
{
    Math::Quatf quat(1.0F, 2.0F, 3.0F, 4.0F);
    quat.normalize();
    auto length = quat.length();

    EXPECT_EQ(length, 1.0F);
}

TEST(Quat, MultiplicationAssociativity)
{
    Math::Quatf a(1.0F, 2.0F, 3.0F, 4.0F);
    Math::Quatf b(5.0F, 6.0F, 7.0F, 8.0F);
    Math::Quatf c(9.0F, 10.0F, 11.0F, 12.0F);

    auto ab_c = (a * b) * c;
    auto a_bc = a * (b * c);

    EXPECT_EQ(ab_c.x, a_bc.x);
    EXPECT_EQ(ab_c.y, a_bc.y);
    EXPECT_EQ(ab_c.z, a_bc.z);
    EXPECT_EQ(ab_c.w, a_bc.w);
}

TEST(Quat, AxisAngleZeroAxis)
{
    Math::Vec3f axis(0.0F, 0.0F, 0.0F);
    auto quat = Math::Quatf::from_axis_angle(axis, 1.0F);
    EXPECT_EQ(quat, Math::Quatf::identity());
}

TEST(Quat, ZRotation)
{
    Math::Vec3f axis(0.0F, 0.0F, 1.0F);
    auto quat = Math::Quatf::from_axis_angle(axis, DEG_TO_RAD(90.0F));

    EXPECT_NEAR(quat.z, 0.7071F, 1e-4F);
    EXPECT_NEAR(quat.w, 0.7071F, 1e-4F);
}

namespace {

void expect_quat_near(Math::Quatf const& actual, Math::Quatf const& expected, f32 tolerance = 1e-5F)
{
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
    EXPECT_NEAR(actual.w, expected.w, tolerance);
}

}

TEST(Quat, Dot)
{
    Math::Quatf a(1.0F, 2.0F, 3.0F, 4.0F);
    Math::Quatf b(5.0F, 6.0F, 7.0F, 8.0F);

    EXPECT_EQ(Math::dot(a, b), 5.0F + 12.0F + 21.0F + 32.0F);
    EXPECT_NEAR(Math::dot(a, a), a.length() * a.length(), 1e-4F);
}

TEST(Quat, Inverse)
{
    auto const quat = Math::Quatf::from_axis_angle({ 0.0F, 1.0F, 0.0F }, DEG_TO_RAD(37.0F));
    expect_quat_near(quat * quat.inverse(), Math::Quatf::identity());

    expect_quat_near(quat.inverse(), quat.conjugate());

    EXPECT_EQ(Math::Quatf(0.0F, 0.0F, 0.0F, 0.0F).inverse(), Math::Quatf::identity());
}

TEST(Quat, ScalarAndSumOperators)
{
    Math::Quatf a(1.0F, 2.0F, 3.0F, 4.0F);
    Math::Quatf b(5.0F, 6.0F, 7.0F, 8.0F);

    expect_quat_near(a * 2.0F, Math::Quatf(2.0F, 4.0F, 6.0F, 8.0F));
    expect_quat_near(a + b, Math::Quatf(6.0F, 8.0F, 10.0F, 12.0F));
    expect_quat_near(b - a, Math::Quatf(4.0F, 4.0F, 4.0F, 4.0F));
    expect_quat_near(-a, Math::Quatf(-1.0F, -2.0F, -3.0F, -4.0F));
}

TEST(Quat, SlerpEndpoints)
{
    auto const from = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(10.0F));
    auto const to = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(100.0F));

    expect_quat_near(Math::slerp(from, to, 0.0F), from);
    expect_quat_near(Math::slerp(from, to, 1.0F), to);
}

TEST(Quat, SlerpIdenticalInputs)
{
    auto const quat = Math::Quatf::from_axis_angle({ 1.0F, 0.0F, 0.0F }, DEG_TO_RAD(42.0F));

    expect_quat_near(Math::slerp(quat, quat, 0.0F), quat);
    expect_quat_near(Math::slerp(quat, quat, 0.5F), quat);
    expect_quat_near(Math::slerp(quat, quat, 1.0F), quat);
}

TEST(Quat, SlerpHalfwayIsTheMidpointRotation)
{
    auto const from = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(0.0F));
    auto const to = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(90.0F));
    auto const expected = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(45.0F));

    expect_quat_near(Math::slerp(from, to, 0.5F), expected);
}

TEST(Quat, SlerpStaysUnitLength)
{
    auto const from = Math::Quatf::from_axis_angle({ 0.3F, 0.5F, 0.8F }, DEG_TO_RAD(15.0F));
    auto const to = Math::Quatf::from_axis_angle({ -0.7F, 0.2F, 0.1F }, DEG_TO_RAD(155.0F));

    for (auto step = 0; step <= 10; ++step) {
        auto const t = static_cast<f32>(step) / 10.0F;
        EXPECT_NEAR(Math::slerp(from, to, t).length(), 1.0F, 1e-5F);
    }
}

TEST(Quat, SlerpTakesTheShortestPath)
{
    auto const from = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(0.0F));
    auto const to = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(90.0F));

    auto const direct = Math::slerp(from, to, 0.25F);
    auto const flipped = Math::slerp(from, -to, 0.25F);

    ASSERT_LT(Math::dot(from, -to), 0.0F);
    expect_quat_near(flipped, direct);
}