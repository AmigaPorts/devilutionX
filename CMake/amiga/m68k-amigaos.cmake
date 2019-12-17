set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# CPU
set(M68K_CPU_TYPES "68000" "68010" "68020" "68040" "68060")
set(M68K_CPU "68000" CACHE STRING "Target CPU model")
set_property(CACHE M68K_CPU PROPERTY STRINGS ${M68K_CPU_TYPES})

# FPU
set(M68K_FPU_TYPES "soft" "hard")
set(M68K_FPU "soft" CACHE STRING "FPU type")
set_property(CACHE M68K_FPU PROPERTY STRINGS ${M68K_FPU_TYPES})

if(NOT M68K_TOOLCHAIN_PATH)
	set(M68K_TOOLCHAIN_PATH /d/amiga-gcc2)
endif()
set(CMAKE_SYSROOT ${M68K_TOOLCHAIN_PATH})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(AMIGA 1)
set(AMIGAOS3 1)
set(PROFILE 0)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")


if(VAMPIRE_V2)
	set(MOVEM_OPT ${M68K_TOOLCHAIN_PATH}/bin/movem_opt.pl)
	set(CMAKE_C_COMPILER_LAUNCHER ${MOVEM_OPT})
	set(CMAKE_CXX_COMPILER_LAUNCHER ${MOVEM_OPT})
	set(CMAKE_CPP_COMPILER_LAUNCHER ${MOVEM_OPT})
	set(CMAKE_ASM_COMPILER_LAUNCHER ${MOVEM_OPT})
endif()

include_directories(SourceX/platform/amiga/include)

set(CMAKE_C_COMPILER ${M68K_TOOLCHAIN_PATH}/bin/m68k-amigaos-gcc)
set(CMAKE_CXX_COMPILER ${M68K_TOOLCHAIN_PATH}/bin/m68k-amigaos-g++)
set(CMAKE_CPP_COMPILER ${M68K_TOOLCHAIN_PATH}/bin/m68k-amigaos-cpp)
set(CMAKE_ASM_COMPILER ${M68K_TOOLCHAIN_PATH}/bin/vasmm68k_mot)
set(CMAKE_PREFIX_PATH ${M68K_TOOLCHAIN_PATH})
if(WIN32)
	set(CMAKE_C_COMPILER ${CMAKE_C_COMPILER}.exe)
	set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER}.exe)
	set(CMAKE_CPP_COMPILER ${CMAKE_CPP_COMPILER}.exe)
	set(CMAKE_ASM_COMPILER ${CMAKE_ASM_COMPILER}.exe)
endif()

# Compiler flags
if(PROFILE)
	set(FLAGS_COMMON "-D__PROFILE__ -pg")
else()
	set(FLAGS_COMMON "-fomit-frame-pointer")
endif()
set(FLAGS_COMMON "${FLAGS_COMMON} -m${M68K_CPU} -m${M68K_FPU}-float -Dfbbb=abcdefghi -Ofast -ffast-math -fdefer-pop -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop -frerun-loop-opt -fregmove -ffast-math -fsingle-precision-constant -fmodulo-sched -fmodulo-sched-allow-regmoves -flive-range-shrinkage -fsched-pressure -fsched-spec-load -fsched-verbose=2 -w -fpermissive -g -noixemul")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_COMMON}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS_COMMON} -D__BIG_ENDIAN__ -D__AMIGA__ -fpermissive")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -quiet -x -m${M68K_CPU} -nowarn=24 -Faout -I${M68K_TOOLCHAIN_PATH}/m68k-amigaos/sys-include ")
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> <DEFINES> <INCLUDES> ${CMAKE_ASM_FLAGS} -o <OBJECT> <SOURCE>")
set(BUILD_SHARED_LIBS OFF)
unset(FLAGS_COMMON)

# Linker configuration
set(CMAKE_EXE_LINKER_FLAGS "-lpthread -lSDL_mixer -Wl,--whole-archive -lSDL -lSDL_ttf  -lpng16 -Wl,--no-whole-archive -lft2 -lz -noixemul -Xlinker --allow-multiple-definition")

# wrapper - replace gcc functions by our own
foreach(_wrapped
	memcpy
	memset
	memcmp)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--wrap=${_wrapped}")
endforeach(_wrapped)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${M68K_TOOLCHAIN_PATH}/m68k-amigaos/libnix/lib/swapstack.o")

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -ldebug ")
# user
set(FREETYPE_INCLUDE_DIRS ${M68K_TOOLCHAIN_PATH}/m68k-amigaos/include)
set(LIBMAD_INCLUDE_DIRS ${M68K_TOOLCHAIN_PATH}/m68k-amigaos/include)

set(ASAN OFF)
set(UBSAN OFF)
set(NONET ON)
set(DEBUG OFF)
set(USE_SDL1 "Use SDL1.2 instead of SDL2" ON)
add_definitions(-D_POSIX_C_SOURCE=200809L)