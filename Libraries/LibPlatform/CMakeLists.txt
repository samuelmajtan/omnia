set(SOURCES
        Input.cpp
)

set(THIRD_PARTY_LIBS)

if (WIN32)
    list(APPEND SOURCES
            Win32/WindowWin32.cpp
    )
endif ()

omnia_lib(LibPlatform
        SOURCES ${SOURCES}
        THIRD_PARTY PRIVATE ${THIRD_PARTY_LIBS}
)
