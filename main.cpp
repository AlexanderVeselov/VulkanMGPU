#include <iostream>
#include <vulkan/vulkan.h>
#define NOMINMAX
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#include <vector>
#include <string>

#define CHECK_RESULT() \
    if (result != VK_SUCCESS)   \
    {                           \
        throw std::runtime_error("VkResult = " + std::to_string(result)); \
    }

namespace
{
    VkInstance vk_instance;
    std::vector<VkPhysicalDevice> physical_devices;
    std::vector<VkDevice> vk_devices;
    std::vector<VkCommandPool> command_pools;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkFence> fences;

    // Semaphore for the first device
    VkSemaphore export_semaphore;
    // Semaphore for the second device
    VkSemaphore import_semaphore;
}

void CreateInstance()
{
    std::vector<char*> instance_extension_names;
    std::vector<char*> instance_layer_names;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.enabledLayerCount = (std::uint32_t)instance_layer_names.size();
    instance_create_info.ppEnabledLayerNames = instance_layer_names.data();
    instance_create_info.enabledExtensionCount = (std::uint32_t)instance_extension_names.size();
    instance_create_info.ppEnabledExtensionNames = instance_extension_names.data();
    VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    CHECK_RESULT();

}

void EnumeratePhysicalDevices()
{
    std::uint32_t physical_device_count;

    VkResult result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);
    CHECK_RESULT();

    physical_devices.resize(physical_device_count);
    result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices.data());
    CHECK_RESULT();
}

void CreateVkDevices()
{
    vk_devices.resize(physical_devices.size());

    std::vector<char*> device_layer_names;
    std::vector<char*> device_extension_names =
    {
        "VK_KHR_external_semaphore_win32"
    };

    for (auto i = 0u; i < physical_devices.size(); ++i)
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.flags = 0;
        queue_create_info.queueFamilyIndex = 0;
        queue_create_info.queueCount = 1;
        float priority = 1.0f;
        queue_create_info.pQueuePriorities = &priority;

        VkDeviceCreateInfo device_create_info = {};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.enabledLayerCount = (std::uint32_t)device_layer_names.size();
        device_create_info.ppEnabledLayerNames = device_layer_names.data();
        device_create_info.enabledExtensionCount = (std::uint32_t)device_extension_names.size();
        device_create_info.ppEnabledExtensionNames = device_extension_names.data();
        device_create_info.pEnabledFeatures = nullptr;

        VkResult result = vkCreateDevice(physical_devices[i], &device_create_info, nullptr, &vk_devices[i]);
        CHECK_RESULT();
    }
}

void CreateCommandPools()
{
    command_pools.resize(physical_devices.size());

    for (auto i = 0u; i < vk_devices.size(); ++i)
    {
        VkCommandPoolCreateInfo command_pool_create_info = {};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.queueFamilyIndex = 0u;

        VkResult result = vkCreateCommandPool(vk_devices[i], &command_pool_create_info, nullptr, &command_pools[i]);
        CHECK_RESULT();
    }
}

void AllocateCommandBuffers()
{
    command_buffers.resize(physical_devices.size());

    for (auto i = 0u; i < vk_devices.size(); ++i)
    {
        VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
        cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buffer_allocate_info.commandPool = command_pools[i];
        cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buffer_allocate_info.commandBufferCount = 1u;
        VkResult result = vkAllocateCommandBuffers(vk_devices[i], &cmd_buffer_allocate_info, &command_buffers[i]);
        CHECK_RESULT();
    }
}

void RecordCommandBuffers()
{
    for (auto i = 0u; i < vk_devices.size(); ++i)
    {

        VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
        cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VkResult result = vkBeginCommandBuffer(command_buffers[i], &cmd_buffer_begin_info);
        CHECK_RESULT();

        // Nothing to do

        result = vkEndCommandBuffer(command_buffers[i]);
        CHECK_RESULT();
    }
}

void CreateFences()
{
    fences.resize(vk_devices.size());

    for (auto i = 0u; i < vk_devices.size(); ++i)
    {
        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkResult result = vkCreateFence(vk_devices[i], &fence_create_info, nullptr, &fences[i]);
        CHECK_RESULT();
    }
}

void CreateSharedSemaphore()
{
    VkDevice vk_export_device = vk_devices[0];

    // Step 1. Create export semaphore from exporting VkDevice
    VkSemaphore vk_export_semaphore = VK_NULL_HANDLE;

    // Need to add VkExportSemaphoreCreateInfo to VkSemaphoreCreateInfo
    VkExportSemaphoreCreateInfo export_semaphore_create_info = {};
    export_semaphore_create_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
    export_semaphore_create_info.pNext = nullptr;
    export_semaphore_create_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    // Create export semaphore itself
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = &export_semaphore_create_info;
    VkResult result = vkCreateSemaphore(vk_export_device, &semaphore_create_info, nullptr, &vk_export_semaphore);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("CreateSharedSemaphore: Failed to create export semaphore (" + std::to_string(result) + ")");
    }

    // Step 2. Export semaphore to a Win32 HANDLE
    HANDLE win32_semaphore_handle = nullptr;

    VkSemaphoreGetWin32HandleInfoKHR win32_get_handle_info = {};
    win32_get_handle_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    win32_get_handle_info.semaphore = vk_export_semaphore;
    win32_get_handle_info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    // Find Win32 HANDLE getter function
    PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR =
        (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(vk_export_device, "vkGetSemaphoreWin32HandleKHR");

    // If it does not present, possibly, VK_KHR_external_semaphore_win32 device extension is disabled
    if (!vkGetSemaphoreWin32HandleKHR)
    {
        throw std::runtime_error("CreateSharedSemaphore: Failed to find vkGetSemaphoreWin32HandleKHR function! "
            "Enable VK_KHR_external_semaphore_win32 extension.");
    }

    // Get HANDLE
    result = vkGetSemaphoreWin32HandleKHR(vk_export_device, &win32_get_handle_info, &win32_semaphore_handle);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("CreateSharedSemaphore: Failed to get Win32 semaphore handle (" + std::to_string(result) + ")");
    }

    // Step 3. Create import semaphore
    VkDevice vk_import_device = vk_devices[1];
    VkSemaphore vk_import_semaphore = VK_NULL_HANDLE;

    semaphore_create_info.pNext = nullptr;
    result = vkCreateSemaphore(vk_import_device, &semaphore_create_info, nullptr, &vk_import_semaphore);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("CreateSharedSemaphore: Failed to create import semaphore (" + std::to_string(result) + ")");
    }

    // Step 4. Import payload from Win32 handle

    VkImportSemaphoreWin32HandleInfoKHR win32_import_semaphore_info = {};
    win32_import_semaphore_info.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
    win32_import_semaphore_info.semaphore = vk_import_semaphore;
    win32_import_semaphore_info.handleType = win32_get_handle_info.handleType;
    win32_import_semaphore_info.handle = win32_semaphore_handle;

    // Find import function
    PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR =
        (PFN_vkImportSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(vk_import_device, "vkImportSemaphoreWin32HandleKHR");

    if (!vkImportSemaphoreWin32HandleKHR)
    {
        throw std::runtime_error("CreateSharedSemaphore: Failed to find vkImportSemaphoreWin32HandleKHR function!");
    }

    result = vkImportSemaphoreWin32HandleKHR(vk_import_device, &win32_import_semaphore_info);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("CreateSharedSemaphore: Failed to import semaphore payload from Win32 handle (" + std::to_string(result) + ")");
    }

    export_semaphore = vk_export_semaphore;
    import_semaphore = vk_import_semaphore;

}

void WaitForFences()
{
    // Wait for fences
    for (auto i = 0u; i < vk_devices.size(); ++i)
    {
        constexpr std::uint64_t kInfiniteWait = std::numeric_limits<std::uint64_t>::max();
        VkResult result = vkWaitForFences(vk_devices[i], 1u, &fences[i], VK_TRUE, kInfiniteWait);
        CHECK_RESULT();
    }
}

void SubmitCommandBuffers()
{
    // Submit to the first device
    {
        VkDevice device = vk_devices[0];
        VkFence fence = fences[0];
        VkCommandBuffer cmd_buffer = command_buffers[0];

        VkQueue queue;
        vkGetDeviceQueue(device, 0u, 0u, &queue);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 0u;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1u;
        submit_info.pCommandBuffers = &cmd_buffer;
        submit_info.signalSemaphoreCount = 1u;
        // Signal to the export semaphore
        submit_info.pSignalSemaphores = &export_semaphore;
        VkResult result = vkQueueSubmit(queue, 1u, &submit_info, fence);
        CHECK_RESULT();
    }

    // Submit to the second device
    {
        VkDevice device = vk_devices[1];
        VkFence fence = fences[1];
        VkCommandBuffer cmd_buffer = command_buffers[1];

        VkQueue queue;
        vkGetDeviceQueue(device, 0u, 0u, &queue);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1u;
        // Wait on the import semaphore
        submit_info.pWaitSemaphores = &import_semaphore;
        VkPipelineStageFlags wait_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        submit_info.pWaitDstStageMask = &wait_mask;
        submit_info.commandBufferCount = 1u;
        submit_info.pCommandBuffers = &cmd_buffer;
        submit_info.signalSemaphoreCount = 0u;
        submit_info.pSignalSemaphores = nullptr;
        VkResult result = vkQueueSubmit(queue, 1u, &submit_info, fence);
        CHECK_RESULT();
    }

}

void RunApplication()
{
    CreateInstance();
    EnumeratePhysicalDevices();
    CreateVkDevices();
    CreateCommandPools();
    AllocateCommandBuffers();
    RecordCommandBuffers();
    CreateFences();
    CreateSharedSemaphore();
    SubmitCommandBuffers();

}

int main(int argc, char** argv)
{
    try
    {
        RunApplication();
    }
    catch (std::exception& ex)
    {
        std::cerr << "Caught exception:\n" << ex.what() << std::endl;
        return -1;
    }

    return 0;
}
