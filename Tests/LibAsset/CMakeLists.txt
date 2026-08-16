set(SOURCES
    Animation.test.cpp
    AssetManager.test.cpp
    AssetRegistry.test.cpp
    AssetSidecar.test.cpp
    AssetTraits.test.cpp
    ImportContext.test.cpp
)

omnia_test(LibAsset
    SOURCES ${SOURCES}
    LIBS LibAsset
)
