set(SOURCES
    ByteStream.test.cpp
    File.test.cpp
    Hash.test.cpp
)

omnia_test(Common
    SOURCES ${SOURCES}
)
