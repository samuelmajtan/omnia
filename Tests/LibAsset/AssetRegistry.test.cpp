/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <Common/File.h>
#include <LibAsset/AssetRegistry.h>
#include <LibAsset/AssetTraits.h>
#include <LibDebug/Logger.h>

namespace {

class Registry : public testing::Test {
protected:
    void SetUp() override
    {
        std::error_code error;
        m_root = std::filesystem::path(testing::TempDir()) / "OmniaRegistryTest";
        std::filesystem::remove_all(m_root, error);
        std::filesystem::create_directories(m_root, error);
        ASSERT_FALSE(error) << error.message();

        m_log_path = std::filesystem::path(testing::TempDir()) / "OmniaRegistryTest.log";
        ASSERT_TRUE(Debug::Logger::initialize({
            .file_path = m_log_path,
            .console_level = Debug::LogLevel::Off,
            .file_level = Debug::LogLevel::Warn,
            .write_latest = false })
                        .has_value());
    }

    void TearDown() override
    {
        Debug::Logger::shutdown();

        std::error_code error;
        std::filesystem::remove_all(m_root, error);
        std::filesystem::remove(m_log_path, error);
    }

    void add_file(std::string_view relative_path) const
    {
        ASSERT_TRUE(File::write_all(m_root / relative_path, "placeholder").has_value());
    }

    std::filesystem::path m_root;
    std::filesystem::path m_log_path;
};

}

TEST_F(Registry, ClassifiesByExtension)
{
    EXPECT_EQ(Asset::asset_type_for("Models/Sponza.gltf"), Asset::AssetType::Model);
    EXPECT_EQ(Asset::asset_type_for("Textures/Error.png"), Asset::AssetType::Texture);
    EXPECT_EQ(Asset::asset_type_for("Textures/Error.jpg"), Asset::AssetType::Texture);
    EXPECT_EQ(Asset::asset_type_for("Shaders/GeometryPass.vs.glsl"), Asset::AssetType::Shader);
    EXPECT_EQ(Asset::asset_type_for("Models/Sponza.bin"), std::nullopt);
    EXPECT_EQ(Asset::asset_type_for("Models/Sponza.gltf.omnia"), std::nullopt);
}

TEST_F(Registry, ScanRegistersImportableFiles)
{
    add_file("Textures/Error.png");
    add_file("Models/Sponza.gltf");
    add_file("Models/Sponza.bin");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 2U);
    EXPECT_TRUE(registry.key_to_id("Textures/Error").has_value());
    EXPECT_TRUE(registry.key_to_id("Models/Sponza").has_value());
}

TEST_F(Registry, ScanWritesSidecars)
{
    add_file("Textures/Error.png");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_TRUE(std::filesystem::exists(m_root / "Textures/Error.png.omnia"));
}

TEST_F(Registry, AssetIDsAreStableAcrossScans)
{
    add_file("Textures/Error.png");
    add_file("Models/Sponza.gltf");

    Asset::AssetRegistry first(m_root);
    ASSERT_TRUE(first.scan().has_value());

    Asset::AssetRegistry second(m_root);
    ASSERT_TRUE(second.scan().has_value());

    ASSERT_EQ(first.entries().size(), 2U);
    for (auto const& [id, entry] : first.entries()) {
        EXPECT_EQ(id, first.key_to_id(entry.key));
        EXPECT_EQ(id, second.key_to_id(entry.key));
    }
}

TEST_F(Registry, ExistingSidecarIsNotRewritten)
{
    add_file("Textures/Error.png");
    auto const sidecar_path = m_root / "Textures/Error.png.omnia";

    Asset::AssetRegistry first(m_root);
    ASSERT_TRUE(first.scan().has_value());
    auto const original = File::read_all(sidecar_path);
    ASSERT_TRUE(original.has_value());

    Asset::AssetRegistry second(m_root);
    ASSERT_TRUE(second.scan().has_value());

    auto const after = File::read_all(sidecar_path);
    ASSERT_TRUE(after.has_value());
    EXPECT_EQ(original.value(), after.value());
}

TEST_F(Registry, MalformedSidecarFailsTheScan)
{
    add_file("Textures/Error.png");
    ASSERT_TRUE(File::write_all(m_root / "Textures/Error.png.omnia", "uuid = garbage\ntype = Texture\n").has_value());

    Asset::AssetRegistry registry(m_root);
    EXPECT_FALSE(registry.scan().has_value());
}

TEST_F(Registry, ResolveRoundTripsThroughID)
{
    add_file("Models/Sponza.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto const id = registry.key_to_id("Models/Sponza");
    ASSERT_TRUE(id.has_value());

    auto const entry = registry.resolve(id.value());
    ASSERT_TRUE(entry.has_value()) << entry.error();
    EXPECT_EQ(entry.value().key, "Models/Sponza");
    EXPECT_EQ(entry.value().type(), Asset::AssetType::Model);
    EXPECT_TRUE(std::holds_alternative<Asset::LooseAssetEntry>(entry.value().source));
}

TEST_F(Registry, ResolveUnknownIDFails)
{
    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());
    EXPECT_FALSE(registry.resolve(Common::UUID::generate()).has_value());
}

TEST_F(Registry, ScanKeepsGoingPastAFileItCannotRead)
{
    add_file("Textures/Error.png");
    add_file("Models/Sponza.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 2U);
    EXPECT_TRUE(registry.key_to_id("Models/Sponza").has_value());
    EXPECT_TRUE(registry.key_to_id("Textures/Error").has_value());
}

TEST_F(Registry, EmptyRootIsNotAnError)
{
    Asset::AssetRegistry registry;
    EXPECT_TRUE(registry.scan().has_value());
}

TEST_F(Registry, MissingRootIsAnError)
{
    Asset::AssetRegistry registry(m_root / "does-not-exist");
    EXPECT_FALSE(registry.scan().has_value());
}

TEST_F(Registry, SubAssetKeyJoinsWithTheSeparator)
{
    Asset::AssetRegistry registry;
    EXPECT_EQ(registry.resolve_sub_asset_key("Models/CesiumMan/CesiumMan", "walk"), "Models/CesiumMan/CesiumMan#walk");
}

TEST_F(Registry, RegisterSubAssetDerivesAStableID)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto const parent = registry.resolve(registry.key_to_id("Models/CesiumMan").value());
    ASSERT_TRUE(parent.has_value()) << parent.error();

    ASSERT_TRUE(registry.register_sub_asset(parent.value(), { .name = "walk", .type = Asset::AssetType::Model }).has_value());

    auto const id = registry.key_to_id("Models/CesiumMan#walk");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), Common::UUID::derive(parent.value().id(), "walk"));
    EXPECT_NE(id.value(), parent.value().id());
}

TEST_F(Registry, SubAssetKeepsTheParentSourcePathAndRemembersItsName)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto const parent = registry.resolve(registry.key_to_id("Models/CesiumMan").value()).value();
    ASSERT_TRUE(registry.register_sub_asset(parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());

    auto const entry = registry.resolve(registry.key_to_id("Models/CesiumMan#walk").value());
    ASSERT_TRUE(entry.has_value()) << entry.error();

    auto const& source = std::get<Asset::LooseAssetEntry>(entry.value().source);
    EXPECT_EQ(source.path, std::get<Asset::LooseAssetEntry>(parent.source).path);
    ASSERT_TRUE(entry.value().sub_asset_name().has_value());
    EXPECT_EQ(entry.value().sub_asset_name().value(), "walk");
    EXPECT_FALSE(parent.sub_asset_name().has_value());
}

TEST_F(Registry, SubAssetIDsAreStableAcrossScans)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry first(m_root);
    ASSERT_TRUE(first.scan().has_value());
    auto const first_parent = first.resolve(first.key_to_id("Models/CesiumMan").value()).value();
    ASSERT_TRUE(first.register_sub_asset(first_parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());

    Asset::AssetRegistry second(m_root);
    ASSERT_TRUE(second.scan().has_value());
    auto const second_parent = second.resolve(second.key_to_id("Models/CesiumMan").value()).value();
    ASSERT_TRUE(second.register_sub_asset(second_parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());

    EXPECT_EQ(first.key_to_id("Models/CesiumMan#walk"), second.key_to_id("Models/CesiumMan#walk"));
}

TEST_F(Registry, RegisterSubAssetRejectsBadNames)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());
    auto const parent = registry.resolve(registry.key_to_id("Models/CesiumMan").value()).value();

    EXPECT_FALSE(registry.register_sub_asset(parent, { .name = "", .type = Asset::AssetType::Model }).has_value());
    EXPECT_FALSE(registry.register_sub_asset(parent, { .name = "walk#run", .type = Asset::AssetType::Model }).has_value());
    EXPECT_EQ(registry.entries().size(), 1U);
}

TEST_F(Registry, SubAssetsCannotNest)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto const parent = registry.resolve(registry.key_to_id("Models/CesiumMan").value()).value();
    ASSERT_TRUE(registry.register_sub_asset(parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());

    auto const sub = registry.resolve(registry.key_to_id("Models/CesiumMan#walk").value()).value();
    EXPECT_FALSE(registry.register_sub_asset(sub, { .name = "faster", .type = Asset::AssetType::Model }).has_value());
}

TEST_F(Registry, DuplicateSubAssetIsRejected)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());
    auto const parent = registry.resolve(registry.key_to_id("Models/CesiumMan").value()).value();

    ASSERT_TRUE(registry.register_sub_asset(parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());
    EXPECT_FALSE(registry.register_sub_asset(parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());
    EXPECT_EQ(registry.entries().size(), 2U);
}

TEST_F(Registry, DisplayNameFallsBackToTheFileStem)
{
    add_file("Models/CesiumMan/CesiumMan.gltf");
    add_file("Shaders/GeometryPass.vs.glsl");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto entry1 = registry.resolve(registry.key_to_id("Models/CesiumMan/CesiumMan").value());
    ASSERT_TRUE(entry1.has_value()) << entry1.error();
    auto entry2 = registry.resolve(registry.key_to_id("Shaders/GeometryPass.vs").value());
    ASSERT_TRUE(entry2.has_value()) << entry2.error();

    EXPECT_EQ(entry1->display_name(), "CesiumMan");
    EXPECT_EQ(entry2->display_name(), "GeometryPass.vs");
}

TEST_F(Registry, SidecarNameOverridesTheDisplayName)
{
    add_file("Models/stormtrooper/scene.gltf");
    Asset::AssetRegistry probe(m_root);
    ASSERT_TRUE(probe.scan().has_value());

    auto const sidecar_path = m_root / "Models/stormtrooper/scene.gltf.omnia";
    auto const existing = File::read_all(sidecar_path);
    ASSERT_TRUE(existing.has_value());
    ASSERT_TRUE(File::write_all(sidecar_path, existing.value() + "name = Stormtrooper\n").has_value());

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto entry = registry.resolve(registry.key_to_id("Models/stormtrooper/scene").value());
    ASSERT_TRUE(entry.has_value()) << entry.error();
    EXPECT_EQ(entry->display_name(), "Stormtrooper");
    EXPECT_TRUE(registry.key_to_id("Models/stormtrooper/scene").has_value());
}

TEST_F(Registry, SubAssetDisplayNameHangsOffTheParent)
{
    add_file("Models/CesiumMan.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());
    auto const parent = registry.resolve(registry.key_to_id("Models/CesiumMan").value()).value();
    ASSERT_TRUE(registry.register_sub_asset(parent, { .name = "walk", .type = Asset::AssetType::Model }).has_value());

    auto entry = registry.resolve(registry.key_to_id("Models/CesiumMan#walk").value());
    ASSERT_TRUE(entry.has_value()) << entry.error();
    EXPECT_EQ(entry->display_name(), "CesiumMan#walk");
    EXPECT_TRUE(registry.key_to_id("Models/CesiumMan#walk").has_value());
}

TEST_F(Registry, RenamingAnAssetDoesNotChangeItsSourceHash)
{
    Asset::AssetSidecar plain(Common::UUID::generate(), Asset::AssetType::Model);

    auto renamed = plain;
    renamed.set_setting(std::string(Asset::AssetSidecar::NAME_SETTING), "Stormtrooper");

    auto tweaked = plain;
    tweaked.set_setting("srgb", "true");

    EXPECT_EQ(plain.hash_settings(1234), renamed.hash_settings(1234));
    EXPECT_NE(plain.hash_settings(1234), tweaked.hash_settings(1234));
}

TEST_F(Registry, DisplayNameStaysShortEvenWhenItIsNotUnique)
{
    add_file("A/Model/scene.gltf");
    add_file("B/Model/scene.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    for (auto const& [id, entry] : registry.entries()) {
        EXPECT_EQ(entry.display_name(), "scene") << "key " << entry.key;
    }
}

TEST_F(Registry, DuplicateKeyIsReportedAsAWarning)
{
    add_file("Textures/Error.png");
    add_file("Textures/Error.jpg");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 1U);
    EXPECT_TRUE(registry.key_to_id("Textures/Error").has_value());
}

namespace {

constexpr std::string_view ANIMATED_GLTF = R"({
  "asset": { "version": "2.0" },
  "scene": 0,
  "scenes": [ { "nodes": [ 0 ] } ],
  "nodes": [ { "name": "root" } ],
  "animations": [
    { "name": "March", "channels": [], "samplers": [] },
    { "name": "Idle", "channels": [], "samplers": [] },
    { "channels": [], "samplers": [] }
  ]
})";

}

TEST_F(Registry, ScanRegistersAnimationsAsSubAssets)
{
    ASSERT_TRUE(File::write_all(m_root / "Models/Clips.gltf", std::string(ANIMATED_GLTF)).has_value());

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 4U);
    for (auto const& name : { "March", "Idle", "Animation_2" }) {
        auto const id = registry.key_to_id(std::format("Models/Clips#{}", name));
        ASSERT_TRUE(id.has_value()) << name;

        auto const entry = registry.resolve(id.value());
        ASSERT_TRUE(entry.has_value()) << entry.error();
        EXPECT_EQ(entry.value().type(), Asset::AssetType::Animation);
        EXPECT_EQ(entry.value().sub_asset_name().value(), name);
    }
}

TEST_F(Registry, AnimationSubAssetsHangOffTheModelTheyShipIn)
{
    ASSERT_TRUE(File::write_all(m_root / "Models/Clips.gltf", std::string(ANIMATED_GLTF)).has_value());

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    auto const parent_id = registry.key_to_id("Models/Clips");
    ASSERT_TRUE(parent_id.has_value());
    auto const parent = registry.resolve(parent_id.value()).value();
    EXPECT_EQ(parent.type(), Asset::AssetType::Model);

    auto const clip = registry.resolve(registry.key_to_id("Models/Clips#March").value()).value();
    EXPECT_EQ(clip.id(), Common::UUID::derive(parent_id.value(), "March"));
    EXPECT_EQ(std::get<Asset::LooseAssetEntry>(clip.source).path, std::get<Asset::LooseAssetEntry>(parent.source).path);
    EXPECT_EQ(clip.display_name(), "Clips#March");
}

TEST_F(Registry, AnimationSubAssetIDsAreStableAcrossScans)
{
    ASSERT_TRUE(File::write_all(m_root / "Models/Clips.gltf", std::string(ANIMATED_GLTF)).has_value());

    Asset::AssetRegistry first(m_root);
    ASSERT_TRUE(first.scan().has_value());

    Asset::AssetRegistry second(m_root);
    ASSERT_TRUE(second.scan().has_value());

    EXPECT_EQ(first.key_to_id("Models/Clips#March"), second.key_to_id("Models/Clips#March"));
    EXPECT_NE(first.key_to_id("Models/Clips#March"), first.key_to_id("Models/Clips#Idle"));
}

TEST_F(Registry, ModelWithoutAnimationsRegistersNoSubAssets)
{
    ASSERT_TRUE(File::write_all(m_root / "Models/Static.gltf", std::string(R"({ "asset": { "version": "2.0" }, "nodes": [] })")).has_value());

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 1U);
    EXPECT_TRUE(registry.key_to_id("Models/Static").has_value());
}
