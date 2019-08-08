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
    std::vector<VkCommandBuffer> command_buffers_1;
    std::vector<VkCommandBuffer> command_buffers_2;
    std::vector<VkFence> fences;

    std::vector<VkSemaphore> internal_semaphores;
    std::vector<VkSemaphore> external_semaphores;

}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void CreateInstance()
{
    std::vector<char*> instance_extension_names =
    {
        "VK_EXT_debug_utils"
    };

    std::vector<char*> instance_layer_names =
    {
        "VK_LAYER_LUNARG_standard_validation"
    };

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.enabledLayerCount = (std::uint32_t)instance_layer_names.size();
    instance_create_info.ppEnabledLayerNames = instance_layer_names.data();
    instance_create_info.enabledExtensionCount = (std::uint32_t)instance_extension_names.size();
    instance_create_info.ppEnabledExtensionNames = instance_extension_names.data();
    VkResult result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    CHECK_RESULT();

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");

    if (!vkCreateDebugUtilsMessengerEXT)
    {
        throw std::runtime_error("Failed to find vkCreateDebugUtilsMessengerEXT function!");
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info = {};
    debug_utils_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_create_info.pfnUserCallback = DebugCallback;
    debug_utils_messenger_create_info.pUserData = nullptr;

    VkDebugUtilsMessengerEXT debug_messenger;
    result = vkCreateDebugUtilsMessengerEXT(vk_instance, &debug_utils_messenger_create_info, nullptr, &debug_messenger);
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
        "VK_KHR_external_semaphore",
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
    command_buffers_1.resize(physical_devices.size());
    command_buffers_2.resize(physical_devices.size());

    for (auto i = 0u; i < vk_devices.size(); ++i)
    {
        VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
        cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buffer_allocate_info.commandPool = command_pools[i];
        cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buffer_allocate_info.commandBufferCount = 1u;
        VkResult result = vkAllocateCommandBuffers(vk_devices[i], &cmd_buffer_allocate_info, &command_buffers_1[i]);
        CHECK_RESULT();

        result = vkAllocateCommandBuffers(vk_devices[i], &cmd_buffer_allocate_info, &command_buffers_2[i]);
        CHECK_RESULT();
    }
}

void RecordCommandBuffers()
{
    for (auto i = 0u; i < vk_devices.size(); ++i)
    {

        VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
        cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        // Cmd buffer 1
        {
            VkResult result = vkBeginCommandBuffer(command_buffers_1[i], &cmd_buffer_begin_info);
            CHECK_RESULT();

            // Nothing to do

            result = vkEndCommandBuffer(command_buffers_1[i]);
            CHECK_RESULT();
        }

        // Cmd buffer 2
        {
            VkResult result = vkBeginCommandBuffer(command_buffers_2[i], &cmd_buffer_begin_info);
            CHECK_RESULT();

            // Nothing to do

            result = vkEndCommandBuffer(command_buffers_2[i]);
            CHECK_RESULT();
        }
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

void CreateInternalSemaphores()
{
    internal_semaphores.resize(vk_devices.size());

    for (auto i = 0u; i < vk_devices.size(); ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult result = vkCreateSemaphore(vk_devices[i], &semaphore_create_info, nullptr, &internal_semaphores[i]);
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

    external_semaphores.resize(2);
    external_semaphores[0] = vk_export_semaphore;
    external_semaphores[1] = vk_import_semaphore;

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

void Submit(VkDevice device, VkCommandBuffer cmd_buffer, std::vector<VkSemaphore> const& wait_semaphores,
    std::vector<VkSemaphore> const& signal_semaphores, VkFence fence)
{
    std::vector<VkPipelineStageFlags> wait_dst_stage_mask;

    for (auto wait_semaphore : wait_semaphores)
    {
        wait_dst_stage_mask.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    }

    VkQueue queue;
    vkGetDeviceQueue(device, 0u, 0u, &queue);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = (std::uint32_t)wait_semaphores.size();
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_dst_stage_mask.data();
    submit_info.commandBufferCount = 1u;
    submit_info.pCommandBuffers = &cmd_buffer;
    submit_info.signalSemaphoreCount = (std::uint32_t)signal_semaphores.size();
    submit_info.pSignalSemaphores = signal_semaphores.data();

    VkResult result = vkQueueSubmit(queue, 1u, &submit_info, fence);
    CHECK_RESULT();

}

void SubmitCommandBuffers()
{
    // Submit 1st cmd buffer to the first device
    {
        std::vector<VkSemaphore> wait_semaphores;
        std::vector<VkSemaphore> signal_semaphores = { internal_semaphores[0] };
        Submit(vk_devices[0], command_buffers_1[0], wait_semaphores, signal_semaphores, VK_NULL_HANDLE);
    }

    // Submit 1st cmd buffer to the second device
    {
        std::vector<VkSemaphore> wait_semaphores;
        // Signal import semaphore
        std::vector<VkSemaphore> signal_semaphores = { internal_semaphores[1], external_semaphores[1] };
        Submit(vk_devices[1], command_buffers_1[1], wait_semaphores, signal_semaphores, fences[1]);
    }

    // Submit 2nd cmd buffer to the first device
    {
        std::vector<VkSemaphore> wait_semaphores = { external_semaphores[0], internal_semaphores[0] };
        std::vector<VkSemaphore> signal_semaphores;
        Submit(vk_devices[0], command_buffers_2[0], wait_semaphores, signal_semaphores, fences[0]);
    }

    // Wait on fences
    WaitForFences();

    // Submit cmd buffers again
    {
        std::vector<VkSemaphore> wait_semaphores;
        std::vector<VkSemaphore> signal_semaphores = { internal_semaphores[0] };
        Submit(vk_devices[0], command_buffers_1[0], wait_semaphores, signal_semaphores, VK_NULL_HANDLE);
    }

    // Submit 1st cmd buffer to the second device
    {
        std::vector<VkSemaphore> wait_semaphores = { internal_semaphores[1] };
        // Signal import semaphore
        std::vector<VkSemaphore> signal_semaphores = { internal_semaphores[1], external_semaphores[1] };
        Submit(vk_devices[1], command_buffers_1[1], wait_semaphores, signal_semaphores, fences[1]);
    }

    // Submit 2nd cmd buffer to the first device
    {
        std::vector<VkSemaphore> wait_semaphores = { external_semaphores[0], internal_semaphores[0] };
        std::vector<VkSemaphore> signal_semaphores;
        Submit(vk_devices[0], command_buffers_2[0], wait_semaphores, signal_semaphores, fences[0]);
    }

    // Wait on fences
    WaitForFences();

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
    CreateInternalSemaphores();
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
