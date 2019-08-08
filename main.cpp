#include <iostream>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#define CHECK_RESULT() \
    if (result != VK_SUCCESS)   \
    {                           \
        throw std::runtime_error("VkResult = " + std::to_string(result)); \
    }

void RunApplication()
{
    VkInstance vk_instance;

    std::vector<char*> instance_extension_names;
    std::vector<char*> instance_layer_names;

    VkResult result = VK_SUCCESS;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.enabledLayerCount = (std::uint32_t)instance_layer_names.size();
    instance_create_info.ppEnabledLayerNames = instance_layer_names.data();
    instance_create_info.enabledExtensionCount = (std::uint32_t)instance_extension_names.size();
    instance_create_info.ppEnabledExtensionNames = instance_extension_names.data();
    result = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    CHECK_RESULT();

    std::uint32_t physical_device_count;
    std::vector<VkPhysicalDevice> physical_devices;
    result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);
    CHECK_RESULT();

    physical_devices.resize(physical_device_count);
    result = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices.data());
    CHECK_RESULT();

    std::vector<VkDevice> vk_devices;
    vk_devices.resize(physical_devices.size());

    std::vector<char*> device_layer_names;
    std::vector<char*> device_extension_names;

    std::vector<VkCommandPool> command_pools;
    command_pools.resize(physical_devices.size());
    std::vector<VkCommandBuffer> command_buffers;
    command_buffers.resize(physical_devices.size());

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

        result = vkCreateDevice(physical_devices[i], &device_create_info, nullptr, &vk_devices[i]);
        CHECK_RESULT();

        VkCommandPoolCreateInfo command_pool_create_info = {};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.queueFamilyIndex = 0u;

        result = vkCreateCommandPool(vk_devices[i], &command_pool_create_info, nullptr, &command_pools[i]);
        CHECK_RESULT();

        VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
        cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buffer_allocate_info.commandPool = command_pools[i];
        cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buffer_allocate_info.commandBufferCount = 1u;
        result = vkAllocateCommandBuffers(vk_devices[i], &cmd_buffer_allocate_info, &command_buffers[i]);
        CHECK_RESULT();

        VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
        cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        result = vkBeginCommandBuffer(command_buffers[i], &cmd_buffer_begin_info);
        CHECK_RESULT();

        result = vkEndCommandBuffer(command_buffers[i]);
        CHECK_RESULT();

        VkQueue queue;
        vkGetDeviceQueue(vk_devices[i], 0u, 0u, &queue);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 0u;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1u;
        submit_info.pCommandBuffers = &command_buffers[i];
        submit_info.signalSemaphoreCount = 0u;
        submit_info.pSignalSemaphores = nullptr;
        result = vkQueueSubmit(queue, 1u, &submit_info, nullptr);
        CHECK_RESULT();
    }

    //VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
    //shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    //shader_stage_create_info.flags;
    //shader_stage_create_info.stage;
    //shader_stage_create_info.module;
    //shader_stage_create_info.pName;
    //shader_stage_create_info.pSpecializationInfo;

    //VkComputePipelineCreateInfo pipeline_create_info = {};
    //pipeline_create_info.flags;
    //pipeline_create_info.stage;
    //pipeline_create_info.layout;
    //pipeline_create_info.basePipelineHandle
    //pipeline_create_info.basePipelineIndex;
    //pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

    //vkCreateComputePipelines()

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
