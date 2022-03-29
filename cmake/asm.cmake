if (WITH_ASM AND NOT XMRIG_ARM AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(XMRIG_ASM_LIBRARY "xmrig-asm")

    enable_language(ASM)

    if (WIN32 AND CMAKE_C_COMPILER_ID MATCHES GNU)
        set(XMRIG_ASM_FILES
            "crypto/asm/win64/cn_main_loop.S"
            "crypto/asm/CryptonightR_template.S"
        )
    else()
        set(XMRIG_ASM_FILES
            "crypto/asm/cn_main_loop.S"
            "crypto/asm/CryptonightR_template.S"
        )
    endif()

    set_property(SOURCE ${XMRIG_ASM_FILES} PROPERTY C)

    add_library(${XMRIG_ASM_LIBRARY} STATIC ${XMRIG_ASM_FILES})
    set(XMRIG_ASM_SOURCES "crypto/CryptonightR_gen.c")
    set_property(TARGET ${XMRIG_ASM_LIBRARY} PROPERTY LINKER_LANGUAGE C)
else()
    set(XMRIG_ASM_SOURCES "")
    set(XMRIG_ASM_LIBRARY "")
    add_definitions(/DXMRIG_NO_ASM)
endif()
