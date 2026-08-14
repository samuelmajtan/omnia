set(SOURCES
        AssetFile.cpp
        AssetManager.cpp
        AssetRegistry.cpp
        AssetSidecar.cpp
        AssetTraits.cpp
        ModelImporter.cpp
        ShaderCompiler.cpp
        ShaderImporter.cpp
        TextureImporter.cpp
)

find_path(CGLTF_INCLUDE_DIRS cgltf.h)
find_package(Stb REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
set(THIRD_PARTY_LIBS unofficial::shaderc::shaderc)

omnia_lib(LibAsset
        SOURCES ${SOURCES}
        LIBS LibPlatform
        THIRD_PARTY ${THIRD_PARTY_LIBS}
        INCLUDE_DIRS PRIVATE ${Stb_INCLUDE_DIR} ${CGLTF_INCLUDE_DIRS}
)