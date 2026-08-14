set(SOURCES
        Logger.test.cpp
)

omnia_test(LibDebug
        SOURCES ${SOURCES}
        LIBS LibDebug
)
