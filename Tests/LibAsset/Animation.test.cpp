/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <LibAsset/AnimationPlayer.h>

namespace {

auto make_skeleton() -> Asset::Skeleton
{
    Graphics::SkeletonData data;
    data.nodes = {
        { .name = "root", .parent_index = -1, .translation = { 0.0F, 0.0F, 0.0F }, .rotation = Math::Quatf::identity(), .scale = { 1.0F, 1.0F, 1.0F } },
        { .name = "hip", .parent_index = 0, .translation = { 0.0F, 1.0F, 0.0F }, .rotation = Math::Quatf::identity(), .scale = { 1.0F, 1.0F, 1.0F } },
        { .name = "knee", .parent_index = 1, .translation = { 0.0F, 2.0F, 0.0F }, .rotation = Math::Quatf::identity(), .scale = { 1.0F, 1.0F, 1.0F } },
    };
    data.bone_nodes = { 1, 2 };
    data.inverse_bind_matrices = {
        Math::Mat4f::translation(0.0F, -1.0F, 0.0F),
        Math::Mat4f::translation(0.0F, -3.0F, 0.0F)
    };
    return Asset::Skeleton(std::move(data));
}

auto make_translation_clip() -> Asset::AnimationClip
{
    return Asset::AnimationClip(Asset::AnimationData {
        .name = "Slide",
        .duration = 2.0F,
        .channels = {
            { .target_node = 1,
                .target_property = Asset::AnimationPath::Translation,
                .interpolation = Asset::AnimationInterpolation::Linear,
                .keyframes = { { .time = 0.0F, .value = { 0.0F, 1.0F, 0.0F, 0.0F } },
                    { .time = 2.0F, .value = { 4.0F, 1.0F, 0.0F, 0.0F } } } } }
    });
}

void expect_translation(Math::Mat4f const& matrix, Math::Vec3f const& expected, f32 tolerance = 1e-5F)
{
    EXPECT_NEAR(matrix.at(0, 3), expected.x, tolerance);
    EXPECT_NEAR(matrix.at(1, 3), expected.y, tolerance);
    EXPECT_NEAR(matrix.at(2, 3), expected.z, tolerance);
}

void expect_maps_to(Math::Mat4f const& matrix, Math::Vec3f const& bind_position, Math::Vec3f const& expected, f32 tolerance = 1e-4F)
{
    auto const skinned = matrix * Math::Vec4f(bind_position, 1.0F);

    EXPECT_NEAR(skinned.x, expected.x, tolerance);
    EXPECT_NEAR(skinned.y, expected.y, tolerance);
    EXPECT_NEAR(skinned.z, expected.z, tolerance);
}

void expect_identity(Math::Mat4f const& matrix, f32 tolerance = 1e-5F)
{
    for (std::size_t index = 0; index < 16; ++index) {
        EXPECT_NEAR(matrix[index], Math::Mat4f::identity()[index], tolerance) << "element " << index;
    }
}

}

TEST(Skeleton, ExposesBonesAndLooksUpNodesByName)
{
    auto const skeleton = make_skeleton();

    EXPECT_EQ(skeleton.node_count(), 3U);
    EXPECT_EQ(skeleton.bone_count(), 2U);
    EXPECT_EQ(skeleton.find_node("knee"), 2U);
    EXPECT_FALSE(skeleton.find_node("tail").has_value());
}

TEST(Pose, StartsAtTheBindPose)
{
    auto const skeleton = make_skeleton();
    Asset::Pose pose(skeleton);

    ASSERT_EQ(pose.size(), 3U);
    EXPECT_EQ(pose.translation(1).y, 1.0F);
    EXPECT_EQ(pose.translation(2).y, 2.0F);
}

TEST(AnimationPlayer, BindPoseWithNoClipGivesIdentityBoneMatrices)
{
    auto const skeleton = make_skeleton();
    Asset::AnimationPlayer player(skeleton);

    ASSERT_EQ(player.bone_matrices().size(), 2U);
    expect_identity(player.bone_matrices()[0]);
    expect_identity(player.bone_matrices()[1]);
}

TEST(AnimationPlayer, ParentMotionPropagatesToChildBones)
{
    auto const skeleton = make_skeleton();
    auto const clip = make_translation_clip();

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.set_time(2.0F);

    expect_translation(player.bone_matrices()[0], { 4.0F, 0.0F, 0.0F });
    expect_translation(player.bone_matrices()[1], { 4.0F, 0.0F, 0.0F });
}

TEST(AnimationPlayer, InterpolatesLinearlyBetweenKeys)
{
    auto const skeleton = make_skeleton();
    auto const clip = make_translation_clip();

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.set_time(0.5F);

    expect_translation(player.bone_matrices()[0], { 1.0F, 0.0F, 0.0F });
}

TEST(AnimationPlayer, StepInterpolationHoldsThePreviousKey)
{
    auto const skeleton = make_skeleton();
    Asset::AnimationClip const clip(Asset::AnimationData {
        .name = "Snap",
        .duration = 2.0F,
        .channels = {
            { .target_node = 1,
                .target_property = Asset::AnimationPath::Translation,
                .interpolation = Asset::AnimationInterpolation::Step,
                .keyframes = { { .time = 0.0F, .value = { 0.0F, 1.0F, 0.0F, 0.0F } },
                    { .time = 2.0F, .value = { 4.0F, 1.0F, 0.0F, 0.0F } } } } }
    });

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);

    player.set_time(1.9F);
    expect_translation(player.bone_matrices()[0], { 0.0F, 0.0F, 0.0F });

    player.set_time(2.0F);
    expect_translation(player.bone_matrices()[0], { 4.0F, 0.0F, 0.0F });
}

TEST(AnimationPlayer, LoopsWithinTheClipDuration)
{
    auto const skeleton = make_skeleton();
    auto const clip = make_translation_clip();

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.update(2.5F);

    EXPECT_NEAR(player.time(), 0.5F, 1e-5F);
    EXPECT_FALSE(player.has_finished());
    expect_translation(player.bone_matrices()[0], { 1.0F, 0.0F, 0.0F });
}

TEST(AnimationPlayer, WrappingProducesTheSamePoseAsTheEquivalentTime)
{
    auto const skeleton = make_skeleton();
    auto const clip = make_translation_clip();

    Asset::AnimationPlayer looped(skeleton);
    looped.set_clip(&clip);
    looped.update(6.5F);

    Asset::AnimationPlayer direct(skeleton);
    direct.set_clip(&clip);
    direct.set_time(0.5F);

    ASSERT_EQ(looped.bone_matrices().size(), direct.bone_matrices().size());
    for (std::size_t bone = 0; bone < looped.bone_matrices().size(); ++bone) {
        for (std::size_t index = 0; index < 16; ++index) {
            EXPECT_NEAR(looped.bone_matrices()[bone][index], direct.bone_matrices()[bone][index], 1e-4F) << "bone " << bone << " element " << index;
        }
    }
}

TEST(AnimationPlayer, NonLoopingClipClampsAtTheEnd)
{
    auto const skeleton = make_skeleton();
    auto const clip = make_translation_clip();

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.set_looping(false);
    player.update(10.0F);

    EXPECT_NEAR(player.time(), 2.0F, 1e-5F);
    EXPECT_TRUE(player.has_finished());
    expect_translation(player.bone_matrices()[0], { 4.0F, 0.0F, 0.0F });
}

TEST(AnimationPlayer, NegativeSpeedWrapsBackwards)
{
    auto const skeleton = make_skeleton();
    auto const clip = make_translation_clip();

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.set_speed(-1.0F);
    player.update(0.5F);

    EXPECT_NEAR(player.time(), 1.5F, 1e-5F);
}

TEST(AnimationPlayer, RotationChannelsUseTheShortestArc)
{
    auto const skeleton = make_skeleton();
    auto const half_turn = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(90.0F));

    Asset::AnimationClip const clip(Asset::AnimationData {
        .name = "Twist",
        .duration = 1.0F,
        .channels = {
            { .target_node = 1,
                .target_property = Asset::AnimationPath::Rotation,
                .interpolation = Asset::AnimationInterpolation::Linear,
                .keyframes = { { .time = 0.0F, .value = { 0.0F, 0.0F, 0.0F, 1.0F } },
                    { .time = 1.0F, .value = { half_turn.x, half_turn.y, half_turn.z, half_turn.w } } } } }
    });

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.set_time(0.5F);

    expect_maps_to(player.bone_matrices()[0], { 0.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F });

    auto const swung = Math::Quatf::from_axis_angle({ 0.0F, 0.0F, 1.0F }, DEG_TO_RAD(45.0F)) * Math::Vec3f(0.0F, 2.0F, 0.0F);
    expect_maps_to(player.bone_matrices()[1], { 0.0F, 3.0F, 0.0F }, swung + Math::Vec3f(0.0F, 1.0F, 0.0F));
}

TEST(AnimationPlayer, ChannelsTargetingUnknownNodesAreIgnored)
{
    auto const skeleton = make_skeleton();
    Asset::AnimationClip const clip(Asset::AnimationData {
        .name = "Stray",
        .duration = 1.0F,
        .channels = {
            { .target_node = 99,
                .target_property = Asset::AnimationPath::Translation,
                .interpolation = Asset::AnimationInterpolation::Linear,
                .keyframes = { { .time = 0.0F, .value = { 0.0F, 0.0F, 0.0F, 0.0F } },
                    { .time = 1.0F, .value = { 9.0F, 9.0F, 9.0F, 0.0F } } } } }
    });

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.update(0.5F);

    expect_identity(player.bone_matrices()[0]);
    expect_identity(player.bone_matrices()[1]);
}

TEST(AnimationPlayer, ZeroDurationClipDoesNotDivideByZero)
{
    auto const skeleton = make_skeleton();
    Asset::AnimationClip const clip(Asset::AnimationData {
        .name = "Still",
        .duration = 0.0F,
        .channels = {
            { .target_node = 1,
                .target_property = Asset::AnimationPath::Translation,
                .interpolation = Asset::AnimationInterpolation::Linear,
                .keyframes = { { .time = 0.0F, .value = { 3.0F, 1.0F, 0.0F, 0.0F } } } } }
    });

    Asset::AnimationPlayer player(skeleton);
    player.set_clip(&clip);
    player.update(1.0F);

    EXPECT_EQ(player.time(), 0.0F);
    expect_translation(player.bone_matrices()[0], { 3.0F, 0.0F, 0.0F });
}
