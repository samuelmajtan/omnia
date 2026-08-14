/*
 *  Copyright (c) 2026, the Omnia developers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gtest/gtest.h>

#include <LibAsset/AssetManager.h>

namespace {

auto cache_root_for(std::string_view source_root) -> std::string
{
    return Asset::AssetManager::default_cache_root(source_root).generic_string();
}

}

TEST(AssetManagerPaths, CacheRootSitsBesideTheSourceTree)
{
    EXPECT_EQ(cache_root_for("Resources"), ".omnia/imported");
    EXPECT_EQ(cache_root_for("Resources/"), ".omnia/imported");
    EXPECT_EQ(cache_root_for("C:/dev/omnia/Resources"), "C:/dev/omnia/.omnia/imported");
    EXPECT_EQ(cache_root_for("C:/dev/omnia/Resources/"), "C:/dev/omnia/.omnia/imported");
    EXPECT_EQ(cache_root_for("some/nested/Assets/"), "some/nested/.omnia/imported");
}

TEST(AssetManagerPaths, EmptySourceRootHasNoCacheRoot)
{
    EXPECT_TRUE(Asset::AssetManager::default_cache_root("").empty());
}

TEST(AssetManagerPaths, ExplicitCacheRootWins)
{
    Asset::AssetManager const manager(Asset::AssetManager::Configuration {
        .source_root = "Resources",
        .cache_root = "somewhere/else" });

    EXPECT_EQ(manager.cooked_asset_cache_root().generic_string(), "somewhere/else");
}

TEST(AssetManagerPaths, CookedPathIsUnderTheCacheRoot)
{
    Asset::AssetManager const manager(Asset::AssetManager::Configuration {
        .source_root = "Resources",
        .cache_root = "cache" });

    auto const id = Common::UUID::generate();
    auto const path = manager.cooked_asset_path(id);

    EXPECT_EQ(path.parent_path().generic_string(), "cache");
    EXPECT_EQ(path.extension().string(), ".oasset");
    EXPECT_NE(path.filename().string().find(id.to_string()), std::string::npos);
}
