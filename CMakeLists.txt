project(VulkanSharedSemaphore CXX)

cmake_minimum_required(VERSION 3.8)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Vulkan REQUIRED)

set(SOURCES
    main.cpp
)

add_executable(VulkanSharedSemaphore ${SOURCES})
target_include_directories(VulkanSharedSemaphore PRIVATE .)
target_link_libraries(VulkanSharedSemaphore PRIVATE Vulkan::Vulkan)
