execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE REVISION
    ERROR_QUIET)

if(NOT REVISION)
    #set(REVISION "Not found")
else()
    string(REPLACE "\r" "" REVISION "${REVISION}")
    string(REPLACE "\n" "" REVISION "${REVISION}")
    string(STRIP "${REVISION}" REVISION)
    #message(WARNING "Subversion found:${REVISION}")
endif()

if (WIN32)

# 将源代码添加到项目的可执行文件
add_executable(dmstack 
    ${CMAKE_SOURCE_DIR}/dmstack/dmstack.cpp
    ${CMAKE_SOURCE_DIR}/dmstack/dmstack.h
    ${CMAKE_SOURCE_DIR}/dmstack/config.cpp
    ${CMAKE_SOURCE_DIR}/dmstack/config.h
    ${CMAKE_SOURCE_DIR}/dmstack/utils/color_print.h
    ${CMAKE_SOURCE_DIR}/dmstack/utils/output.h
    ${CMAKE_SOURCE_DIR}/dmstack/utils/util.h
    ${CMAKE_SOURCE_DIR}/dmstack/log/log.h
)

set_target_properties(dmstack PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Windows
if (MSVC)
    target_compile_options(dmstack PRIVATE /GS)          # 栈保护（MSVC）
else()
    target_compile_options(dmstack PRIVATE -fstack-protector-all)  # MinGW/Clang on Windows
endif()

target_include_directories(dmstack PRIVATE ${CMAKE_SOURCE_DIR}/dmstack)

target_compile_definitions(dmstack PRIVATE "REVISION=\"${REVISION}\"")

else()
# 获取所有源代码
file(GLOB_RECURSE CPP_FILES "${CMAKE_SOURCE_DIR}/dmstack/*.cpp")
file(GLOB_RECURSE H_FILES "${CMAKE_SOURCE_DIR}/dmstack/*.h")

# 将源代码添加到项目的可执行文件
add_executable(dmstack ${CPP_FILES} ${H_FILES})

target_compile_options(dmstack PRIVATE -fstack-protector-all -fPIE)
target_include_directories(dmstack PRIVATE
    ${CMAKE_SOURCE_DIR}/dmstack
)

EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)

find_package(Threads REQUIRED)
find_library(BFD_LIBRARY NAMES bfd REQUIRED)
find_library(IBERTY_LIBRARY NAMES iberty REQUIRED)
find_library(ELF_LIBRARY NAMES elf REQUIRED)
find_library(UNWIND_LIBRARY NAMES unwind REQUIRED)
find_library(UNWIND_PTRACE_LIBRARY NAMES unwind-ptrace REQUIRED)
find_library(UNWIND_ARCH_LIBRARY NAMES unwind-${ARCHITECTURE} REQUIRED)


target_link_options(dmstack PRIVATE -Wl,-z,relro,-z,now -pie)
target_link_libraries(dmstack
    PRIVATE
    bfd
    iberty
    elf
    unwind-ptrace
    unwind-${ARCHITECTURE}
    unwind
    Threads::Threads
    dl
    rt
    z
)

target_compile_definitions(dmstack PRIVATE "REVISION=\"${REVISION}\"")
endif()
# TODO: 如有需要，请添加测试并安装目标.
