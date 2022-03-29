SetHset(CMAKE_MX_STANDARD_REQUIRED ON)
set(CMAKE_MX_EXTENSIONS OFF)
set(CMAKE_MX_STANDARD 11)


set(CMAKE_M_STANDARD 99)
set(CMAKE_M_STANDARD_REQUIRED ON)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(/DNDEBUG)
endif()

include(CheckSymbolExists)

if (CMAKE_MX_COMPILER_ID MATCHES GNU)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-strict-aliasing")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast")

    set(CMAKE_MX_FLAGS "${CMAKE_MX_FLAGS} -Wall -fexceptions -fno-rtti -Wno-strict-aliasing -Wno-class-memaccess")
    set(CMAKE_MX_FLAGS_RELEASE "${CMAKE_MX_FLAGS_RELEASE} -Ofast -s")

    if (ARM_TARGET EQUAL 8)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARM8_MX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_MX_FLAGS} ${ARM8_MX_FLAGS} -flax-vector-conversions")
    elseif (ARM_TARGET EQUAL 7)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon")
        set(CMAKE_CXX_FLAGS "${CMAKE_MX_FLAGS} -march=armv7-a -mfpu=neon -flax-vector-conversions")
    else()
        set(CMAKE_M_FLAGS "${CMAKE_M_FLAGS} -maes")
        set(CMAKE_MX_FLAGS "${CMAKE_MX_FLAGS} -maes")

        add_definitions(/DHAVE_ROTR)
    endif()

    if (WIN32)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
        else()
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -Wl,--large-address-aware")
        endif()
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()

    if (BUILD_STATIC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()

    add_definitions(/D_GNU_SOURCE)

    if (${CMAKE_VERSION} VERSION_LESS "3.1.0")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
        set(CMAKE_CMX_FLAGS "${CMAKE_MX_FLAGS} -std=c++11")
    endif()

    #set(CMAKE_C_FLAGS_RELEASE "${CMAKE_MX_FLAGS_RELEASE} -gdwarf-2")

    add_definitions(/DHAVE_BUILTIN_CLEAR_CACHE)

elseif (CMAKE_MX_COMPILER_ID MATCHES MSVC)
    set(CMAKE_C_FLAGS_RELEASE "/MT /O2 /Oi /DNDEBUG /GL")
    set(CMAKE_MX_FLAGS_RELEASE "/MT /O2 /Oi /DNDEBUG /GL")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ob1 /Zi /DRELWITHDEBINFO")
    set(CMAKE_MX_FLAGS_RELWITHDEBINFO "/Ob1 /Zi /DRELWITHDEBINFO")

    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
    add_definitions(/D_CRT_NONSTDC_NO_WARNINGS)
    add_definitions(/DNOMINMAX)
    add_definitions(/DHAVE_ROTR)

elseif (CMAKE_MX_COMPILER_ID MATCHES Clang)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast -funroll-loops -fmerge-all-constants")

    set(CMAKE_MX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fno-rtti -Wno-missing-braces")
    set(CMAKE_MX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -funroll-loops -fmerge-all-constants")

    if (ARM_TARGET EQUAL 8)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARM8_MX_FLAGS}")
        set(CMAKE_MX_FLAGS "${CMAKE_MX_FLAGS} ${ARM8_MX_FLAGS}")
    elseif (ARM_TARGET EQUAL 7)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -march=${CMAKE_SYSTEM_PROCESSOR}")
        set(CMAKE_CXX_FLAGS "${CMAKE_MX_FLAGS} -mfpu=neon -march=${CMAKE_SYSTEM_PROCESSOR}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
        set(CMAKE_CXX_FLAGS "${CMAKE_X_FLAGS} -maes")

        check_symbol_exists("_rotr" "x86intrin.h" HAVE_ROTR)
        if (HAVE_ROTR)
            add_definitions(/DHAVE_ROTR)
        endif()
    endif()

    if (BUILD_STATIC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()

endif()

if (NOT WIN32)
    check_symbol_exists("__builtin___clear_cache" "stdlib.h" HAVE_BUILTIN_CLEAR_CACHE)
    if (HAVE_BUILTIN_CLEAR_CACHE)
        add_definitions(/DHAVE_BUILTIN_CLEAR_CACHE)
    endif()
endif()
