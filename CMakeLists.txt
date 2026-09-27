cmake_minimum_required(VERSION 3.16)
project(machgate C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    message(FATAL_ERROR "MachGate requires an ARM64 Linux target")
endif()

add_executable(machgate
    src/machgate.c
    src/loader.c
    src/native_imports.c
    src/native_provider_map.c
    src/guest_symbols.c
    src/guest_allocator_bridge.c
    src/trampoline.c
    src/guest_call.S
    src/vm_interpose_loader.c
    src/syscall/syscall_gate.c
    src/syscall/guest_vm_dispatch.c
    src/syscall/guest_vm_track.c
    src/syscall/syscall_range_000_099.c
    src/syscall/syscall_range_100_199.c
    src/syscall/syscall_range_200_399.c
    src/syscall/syscall_range_400_plus.c
    src/syscall/execve_reexec.c
)

target_include_directories(machgate PRIVATE src)
target_compile_definitions(machgate PRIVATE _GNU_SOURCE)
target_compile_options(machgate PRIVATE -Wall -Wextra -O2 -g)
target_link_libraries(machgate PRIVATE dl pthread)
target_link_options(machgate PRIVATE -rdynamic)

add_library(vm_interpose SHARED src/vm_interpose.c)
set_target_properties(vm_interpose PROPERTIES
    OUTPUT_NAME "machgate_vm_interpose"
    PREFIX "lib"
)
target_compile_options(vm_interpose PRIVATE -Wall -O2 -g)
target_compile_definitions(vm_interpose PRIVATE _GNU_SOURCE)
target_link_libraries(vm_interpose PRIVATE dl)

add_library(system_shim SHARED
    src/shim/libsystem_shim.c
    src/syscall/execve_reexec.c
)
set_target_properties(system_shim PROPERTIES
    OUTPUT_NAME "system_shim"
    PREFIX "lib"
)
target_compile_options(system_shim PRIVATE -Wall -O2 -g -fno-omit-frame-pointer)
target_link_libraries(system_shim PRIVATE m pthread dl)
target_link_options(system_shim PRIVATE
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/shim/libsystem_shim.ver
)

install(TARGETS machgate RUNTIME DESTINATION bin)
install(TARGETS vm_interpose LIBRARY DESTINATION lib)
install(TARGETS system_shim LIBRARY DESTINATION lib)
