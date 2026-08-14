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

        // scan() reports rejected entries through the logger, so route it to a file we
        // can read back. Kept beside m_root rather than inside it, so scan cannot see it.
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

    auto captured_warnings() const -> std::string
    {
        Debug::Logger::shutdown();
        return File::read_all(m_log_path).value_or(std::string {});
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
    for (auto const& [key, entry] : first.entries()) {
        EXPECT_EQ(second.key_to_id(key), entry.id()) << "key " << key;
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

TEST_F(Registry, DuplicateKeyIsReportedAsAWarning)
{
    add_file("Textures/Error.png");
    add_file("Textures/Error.jpg");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 1U);

    auto const logged = captured_warnings();
    EXPECT_NE(logged.find("Textures/Error"), std::string::npos);
    EXPECT_NE(logged.find("WARN"), std::string::npos);
}

TEST_F(Registry, CleanScanHasNoWarnings)
{
    add_file("Textures/Error.png");
    add_file("Models/Sponza.gltf");

    Asset::AssetRegistry registry(m_root);
    ASSERT_TRUE(registry.scan().has_value());

    EXPECT_EQ(registry.entries().size(), 2U);
    EXPECT_TRUE(captured_warnings().empty());
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
