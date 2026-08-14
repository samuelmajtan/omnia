set(SOURCES
    ByteStream.test.cpp
    File.test.cpp
    Hash.test.cpp
    Random.test.cpp
    String.test.cpp
    UUID.test.cpp
)

omnia_test(Common
    SOURCES ${SOURCES}
)
