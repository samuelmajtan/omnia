set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wcast-qual -Wformat=2)
    add_compile_options(-Wno-gnu-statement-expression)
endif()

if (OMNIA_ENABLE_WERROR AND CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    add_compile_options(-Werror)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    add_compile_options(-fno-exceptions)
endif()

if (OMNIA_ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fno-omit-frame-pointer -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

if (WIN32)
    add_compile_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

add_compile_definitions(
    $<$<CONFIG:Debug>:OA_BUILD_DEBUG>
    $<$<CONFIG:Release>:OA_BUILD_DISTRIBUTION>
    $<$<CONFIG:RelWithDebInfo>:OA_BUILD_RELWITHDEBINFO>
)