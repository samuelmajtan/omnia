set(SOURCES
        AnimatedModel.cpp
        Camera.cpp
        DeferredRenderer.cpp
        Material.cpp
        Model.cpp
        ResourceManager.cpp
        SubMesh.cpp
)

omnia_lib(LibRenderer
        SOURCES ${SOURCES}
        LIBS LibRHI LibAsset LibPlatform
)
