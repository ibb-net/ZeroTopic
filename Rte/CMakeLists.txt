
cmake_minimum_required(VERSION 3.15)

set(NOW_TARGET os)

unset(NOW_SOURCES)
unset(NOW_INCLUDE_DIR)
unset(NOW_LINK_DIR)
unset(NOW_LINK_LIBRARIES)

list(APPEND NOW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc)
list(APPEND NOW_INCLUDE_DIR ${CMAKE_BINARY_DIR})

# os_interrupt
if("${CONFIG_OS_INTERFACE_INTERRUPT}" STREQUAL "y")
list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/os_interrupt.c)
endif()

# os_init
if("${CONFIG_OS_INTERFACE_INIT}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/os_init.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/linux/os_init.c)
elseif("${CONFIG_OS}" STREQUAL "baremetal")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/baremetal/os_init.c)
endif()
endif()

# os_heap
if("${CONFIG_OS_INTERFACE_HEAP}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_heap.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_heap.c)
elseif("${CONFIG_OS}" STREQUAL "baremetal")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/baremetal/tlsf.c)
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/baremetal/os_heap.c)
endif()
endif()

# os_printf
if("${CONFIG_OS_INTERFACE_PRINTF}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_printf.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_printf.c)
elseif("${CONFIG_OS}" STREQUAL "baremetal")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_printf.c)
endif()
endif()

# os_mmap
if("${CONFIG_OS_INTERFACE_MMAP}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/os_mmap.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/linux/os_mmap.c)
endif()
endif()

# os_thread
if("${CONFIG_OS_INTERFACE_THREAD}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/os_thread.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_thread.c)
endif()
endif()

# os_mutex
if("${CONFIG_OS_INTERFACE_MUTEX}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/os_mutex.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/linux/os_mutex.c)
elseif("${CONFIG_OS}" STREQUAL "baremetal")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/baremetal/os_mutex.c)
endif()
endif()

# os_semaphore
if("${CONFIG_OS_INTERFACE_SEMAPHORE}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/os_semaphore.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/linux/os_semaphore.c)
elseif("${CONFIG_OS}" STREQUAL "baremetal")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/baremetal/os_semaphore.c)
endif()
endif()

# os_file
if("${CONFIG_OS_INTERFACE_FILE}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_file.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/linux/os_file.c)
endif()
endif()

# os_folder
if("${CONFIG_OS_INTERFACE_FOLDER}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/os_folder.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/linux/os_folder.c)
endif()
endif()

# os_time
if("${CONFIG_OS_INTERFACE_TIMESTAMP}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_timestamp.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_timestamp.c)
endif()
endif()

# os_timer
if("${CONFIG_OS_INTERFACE_TIMER}" STREQUAL "y")
if("${CONFIG_OS}" STREQUAL "windows")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_timer_thread.c)
elseif("${CONFIG_OS}" STREQUAL "linux")
    list(APPEND NOW_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix/os_timer_thread.c)
endif()
endif()

add_library(${NOW_TARGET} STATIC ${NOW_SOURCES})
target_include_directories(${NOW_TARGET} PUBLIC ${NOW_INCLUDE_DIR})
target_link_directories(${NOW_TARGET}    PUBLIC ${NOW_LINK_DIR})
target_link_libraries(${NOW_TARGET}      PUBLIC ${NOW_LINK_LIBRARIES})

# os_interrupt
if("${CONFIG_OS_INTERFACE_INTERRUPT}" STREQUAL "y")
install(FILES inc/os_interrupt.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_init
if("${CONFIG_OS_INTERFACE_INIT}" STREQUAL "y")
install(FILES inc/os_init.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_heap
if("${CONFIG_OS_INTERFACE_HEAP}" STREQUAL "y")
    install(FILES inc/os_heap.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_printf
if("${CONFIG_OS_INTERFACE_PRINTF}" STREQUAL "y")
    install(FILES inc/os_printf.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_mmap
if("${CONFIG_OS_INTERFACE_MMAP}" STREQUAL "y")
    install(FILES inc/os_mmap.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_thread
if("${CONFIG_OS_INTERFACE_THREAD}" STREQUAL "y")
    install(FILES inc/os_thread.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_mutex
if("${CONFIG_OS_INTERFACE_MUTEX}" STREQUAL "y")
    install(FILES inc/os_mutex.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_semaphore
if("${CONFIG_OS_INTERFACE_SEMAPHORE}" STREQUAL "y")
    install(FILES inc/os_semaphore.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_file
if("${CONFIG_OS_INTERFACE_FILE}" STREQUAL "y")
    install(FILES inc/os_file.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_folder
if("${CONFIG_OS_INTERFACE_FOLDER}" STREQUAL "y")
    install(FILES inc/os_folder.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_time
if("${CONFIG_OS_INTERFACE_TIMESTAMP}" STREQUAL "y")
    install(FILES inc/os_timestamp.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

# os_timer
if("${CONFIG_OS_INTERFACE_TIMER}" STREQUAL "y")
    install(FILES inc/os_timer.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

install(TARGETS ${NOW_TARGET} ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
