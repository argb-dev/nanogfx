#include <vector>
#include <fstream>
#include <stdio.h>
#define NG_DEBUG
#define NG_IMPLEMENT
#define NG_VULKAN_SUPPORT
#include "nanogfx.h"
static nanoGFX::nGSurface app;
static bool done = false;
#define _dbg(...) { fprintf( stderr,"=>[%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

#define _err(...) { fprintf( stderr,"[ERROR][%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

struct Vertex {
    float pos[3];
    float color[4];
};

const Vertex verts[] = {
    {{-0.1,-0.1,0},{1,0,0,1}},
    {{0.1,-0.1,0},{1,0,1,1}},
    {{0.1,0.1,0},{1,1,0,1}},
    {{-0.1,-0.1,0},{1,0,0,1}},
    {{0.1,0.1,0},{1,1,0,1}},
    {{-0.1,0.1,0},{1,1,1,1}}
};

static void init(int w, int h)
{
}

static void draw()
{
}


static void eventHandler(const nanoGFX::nGSurface& surface, const nanoGFX::nGEvent& ev, void* user_data)
{
    if(ev.type == nanoGFX::nGEvent::WINDOW_CREATE) init(app.getWidth(), app.getHeight());
    if(ev.type == nanoGFX::nGEvent::WINDOW_REPAINT) draw();

    if(ev.type == nanoGFX::nGEvent::WINDOW_DESTROY || ev.type == nanoGFX::nGEvent::KEY_DOWN) {
        done = true;
    }
}

int main(int argc, char* argv[])
{
    int result = -1;
    app.setEventHandler(eventHandler,NULL);
    app.create(400,400, nanoGFX::SURFACE_NONE);
  
    VkInstance vkInstance = NULL;
    VkSurfaceKHR vkSurface = NULL;
    VkPhysicalDevice vkPhyDevice = NULL;
    VkDevice vkDevice = NULL;
    VkQueue graphicsQueue, presentQueue;
    VkSwapchainKHR vkSwapchain = NULL;
    std::vector<VkImage> scImages;
    std::vector<VkImageView> scImageViews;
    VkFormat scFormat;
    VkExtent2D scExtent;
    VkShaderModule vShader, fShader;
    VkViewport viewport = {};
    VkPipelineLayout pipelineLayout =  NULL;
    VkRenderPass renderPass = NULL;
    VkPipeline graphicsPipeline = NULL;
    std::vector<VkFramebuffer> vkFb;
    VkCommandPool commandPool = NULL;
    VkBuffer vertexBuffer = NULL;
    VkDeviceMemory vbMemory;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageSem = NULL, rdoneSem = NULL;

    uint32_t graphics_family = UINT32_MAX, present_family = UINT32_MAX;
    
    VkInstanceCreateInfo vkCInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL, 0, NULL, 0, NULL, 0, NULL};
    VkXlibSurfaceCreateInfoKHR info = {};
    vkCInfo.ppEnabledExtensionNames = app.getVulkanRequiredExtensions(vkCInfo.enabledExtensionCount);

    if(vkCreateInstance(&vkCInfo, NULL, &vkInstance) == VK_SUCCESS && 
       app.createVulkanSurface(vkInstance, &vkSurface) == 0) {
        uint32_t devCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &devCount, NULL);
        if(devCount > 0 ){
            std::vector<VkPhysicalDevice> devs(devCount);
            vkEnumeratePhysicalDevices(vkInstance, &devCount, devs.data());
            for(size_t i = 0; i < devs.size(); ++i) {
                VkPhysicalDeviceProperties props;
                VkPhysicalDeviceFeatures feats;
                vkGetPhysicalDeviceProperties(devs[i], &props);
                vkGetPhysicalDeviceFeatures(devs[i], &feats);
                if(feats.geometryShader) {
                    uint32_t qcount = 0;
                    vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &qcount, NULL);
                    std::vector<VkQueueFamilyProperties> qFamilies(qcount);
                    vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &qcount, qFamilies.data());
                    for(size_t q = 0; q < qFamilies.size(); ++q) {
                        if(qFamilies[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphics_family = q;
                        VkBool32 present = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR(devs[i], q, vkSurface, &present);
                        if(present) present_family = q;
                    }
                    vkPhyDevice = devs[i];
                    break;
                }
            }
        }
        if(vkPhyDevice && graphics_family != UINT32_MAX) {
             uint32_t extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(vkPhyDevice, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(vkPhyDevice, nullptr, &extensionCount, availableExtensions.data());
            for(size_t i = 0; i < extensionCount; ++i) {
                _dbg("[%s]", availableExtensions[i].extensionName);
            }
            _dbg("gfamily: %u, presfamily: %u", graphics_family, present_family);
            std::vector<VkDeviceQueueCreateInfo> qcInfo(graphics_family == present_family? 1: 2);
            uint32_t families[2]= {graphics_family, present_family};
            float priority = 1.f;
            for(size_t i = 0; i < qcInfo.size(); ++i) {
                qcInfo[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                qcInfo[i].queueFamilyIndex = families[i];
                qcInfo[i].queueCount = 1;
                qcInfo[i].pQueuePriorities = &priority;
            }

            const char* enabledExtensions[] = {"VK_KHR_swapchain"};
            VkDeviceCreateInfo devInfo = {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, NULL, 0,
                (uint32_t)qcInfo.size(), qcInfo.data(), 0, NULL, 
                sizeof(enabledExtensions)/sizeof(enabledExtensions[0]), enabledExtensions, 
                NULL
            };
            vkCreateDevice(vkPhyDevice, &devInfo, NULL, &vkDevice);

            if(vkDevice) {
                vkGetDeviceQueue(vkDevice, graphics_family, 0, &graphicsQueue);
                vkGetDeviceQueue(vkDevice, present_family, 0, &presentQueue);
                if(graphicsQueue && presentQueue) {
                    VkSurfaceCapabilitiesKHR caps;
                    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhyDevice, vkSurface, &caps);

                    VkSwapchainCreateInfoKHR sinfo = {};
                    sinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                    sinfo.surface = vkSurface;
                    sinfo.minImageCount = 2;
                    sinfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
                    sinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                    if(caps.currentExtent.width != UINT32_MAX) {
                        sinfo.imageExtent = caps.currentExtent;
                    }
                    else {
                        sinfo.imageExtent = {app.getWidth(), app.getHeight()};
                    }
                    sinfo.imageArrayLayers = 1;
                    sinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                    if(graphics_family != present_family) {
                        sinfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                        sinfo.queueFamilyIndexCount = 2;
                        sinfo.pQueueFamilyIndices = families;
                    }
                    else {
                        sinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                        sinfo.queueFamilyIndexCount = 0;
                        sinfo.pQueueFamilyIndices = NULL;
                    }
                    sinfo.preTransform = caps.currentTransform;
                    sinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                    sinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
                    sinfo.clipped = VK_TRUE;
                    sinfo.oldSwapchain = VK_NULL_HANDLE;
                    scExtent = sinfo.imageExtent;
                    scFormat = sinfo.imageFormat;
                    vkCreateSwapchainKHR(vkDevice, &sinfo, NULL, &vkSwapchain);

                    if(vkSwapchain) {
                        uint32_t count = 0;
                        vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &count, NULL);
                        if(count > 0) {
                            scImages.resize(count, NULL);
                            vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &count, scImages.data());
                            _dbg("images: %d",count);
                            scImageViews.resize(count);
                            for(size_t i = 0; i < count; ++i) {
                                VkImageViewCreateInfo cinfo = {
                                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0,
                                    scImages[i],  VK_IMAGE_VIEW_TYPE_2D, scFormat,
                                    {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                     VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
                                    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
                                };
                                vkCreateImageView(vkDevice, &cinfo, NULL, &scImageViews[i]);
                            }
                            if(scImageViews[0]) {
                                _dbg("!")
                                std::vector<uint8_t> vprog,fprog;
                                struct Reader {
                                    Reader(std::vector<uint8_t>& dest, const char* fname) {
                                        std::ifstream f(fname, std::ios::ate | std::ios::binary);
                                        if(f.is_open()) {
                                            dest.resize(f.tellg());
                                            f.seekg(0);
                                            f.read((char*)dest.data(), dest.size());
                                            f.close();
                                        }
                                    }
                                } vr(vprog, "v.spv"), fr(fprog, "f.spv");
                                VkShaderModuleCreateInfo cinfo = {
                                    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, vprog.size(), (const uint32_t*) vprog.data()};
                                vkCreateShaderModule(vkDevice, &cinfo, NULL, &vShader);
                                cinfo.codeSize = fprog.size();
                                cinfo.pCode = (const uint32_t*) fprog.data();
                                vkCreateShaderModule(vkDevice, &cinfo, NULL, &fShader);
                                
                                VkPipelineShaderStageCreateInfo vinfo = {
                                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, 
                                    VK_SHADER_STAGE_VERTEX_BIT, vShader, "main", 0
                                };
                                VkPipelineShaderStageCreateInfo finfo = {
                                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, 
                                    VK_SHADER_STAGE_FRAGMENT_BIT, fShader, "main", 0
                                };

                                VkPipelineShaderStageCreateInfo sinfo[] = {vinfo, finfo};


                                VkVertexInputBindingDescription vdesc = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
                                VkVertexInputAttributeDescription vinputdesc = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
                                VkVertexInputAttributeDescription cinputdesc = {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 3*sizeof(float)};

                                VkVertexInputAttributeDescription  inputDesc[2] = {vinputdesc,cinputdesc};
                                VkPipelineVertexInputStateCreateInfo vertexInfo = {
                                    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL,0,
                                    1, &vdesc, 2 ,inputDesc
                                };


                                VkBufferCreateInfo bufferInfo = {
                                    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0,
                                    sizeof(Vertex) * 6, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                    VK_SHARING_MODE_EXCLUSIVE, 0,NULL
                                };
                                vkCreateBuffer(vkDevice, &bufferInfo, NULL, &vertexBuffer);
                                VkMemoryRequirements memReq;
                                vkGetBufferMemoryRequirements(vkDevice, vertexBuffer, &memReq);
                                VkPhysicalDeviceMemoryProperties memProps;
                                vkGetPhysicalDeviceMemoryProperties(vkPhyDevice, &memProps);
                                uint32_t memIdx = UINT32_MAX;
                                uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                                for(uint32_t i = 0 ; i< memProps.memoryTypeCount; ++i) {
                                    _dbg("memtypebits: %#x %#x, props: %#x, %#x", memReq.memoryTypeBits, 1<< i, memProps.memoryTypes[i].propertyFlags, flags)
                                    if((memReq.memoryTypeBits & (1 << i)) && ((memProps.memoryTypes[i].propertyFlags & flags) == flags)) {
                                        memIdx = i;
                                        break;
                                    }
                                }

                                if(memIdx != UINT32_MAX) {
                                    VkMemoryAllocateInfo allocInfo ={ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, memReq.size, memIdx};
                                    vkAllocateMemory(vkDevice, &allocInfo, NULL, &vbMemory);
                                    vkBindBufferMemory(vkDevice, vertexBuffer, vbMemory, 0);
                                    void* ptr = NULL;
                                    vkMapMemory(vkDevice, vbMemory, 0, bufferInfo.size, 0, &ptr);
                                    _dbg("bufferinfosize: %d", bufferInfo.size);
                                    memcpy(ptr, (void*)verts, (size_t) bufferInfo.size);
                                    vkUnmapMemory(vkDevice, vbMemory);
                                }

                                VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
                                    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0, 
                                    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
                                };
                                viewport.x = 0.0f;
                                viewport.y = 0.0f;
                                viewport.width = (float) scExtent.width;
                                viewport.height = (float) scExtent.height;
                                viewport.minDepth = 0.0f;
                                viewport.maxDepth = 1.0f;
                                VkRect2D scissor = { {0,0}, scExtent};
                                VkPipelineViewportStateCreateInfo viewportState = {
                                    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0,
                                    1, &viewport, 1, &scissor
                                };
                                VkPipelineRasterizationStateCreateInfo rasterizer = {
                                    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, NULL, 0,
                                    VK_FALSE, VK_FALSE, 
                                    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE,
                                    VK_FALSE, 0.f, 0.f, 0.f, 1.f
                                };

                                
                                VkPipelineMultisampleStateCreateInfo multisampling = {
                                    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, NULL, 0,
                                    VK_SAMPLE_COUNT_1_BIT, VK_FALSE, 1.f, NULL, VK_FALSE, VK_FALSE
                                };

                                VkPipelineColorBlendAttachmentState colorBlendAttachment = {
                                    VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                    VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO,
                                    VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
                                };

                                VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, NULL, 0, 0, NULL, 0, NULL};
                                vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, NULL, &pipelineLayout);
                                VkAttachmentReference colorAttachmentRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

                                VkSubpassDescription subpass = {};
                                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                                subpass.colorAttachmentCount = 1;
                                subpass.pColorAttachments = &colorAttachmentRef;

                                VkAttachmentDescription colorAttachment = {
                                    0, scFormat, VK_SAMPLE_COUNT_1_BIT, 
                                    VK_ATTACHMENT_LOAD_OP_CLEAR,  VK_ATTACHMENT_STORE_OP_STORE,
                                    VK_ATTACHMENT_LOAD_OP_DONT_CARE ,VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                };
                                
                                VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, NULL, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, {0,0,0,0}};

                                VkSubpassDependency dependency = {
                                    VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0
                                };
                                VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, NULL, 0, 1, &colorAttachment, 1, &subpass, 1, &dependency};
                                vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &renderPass);

                                if(renderPass) {
                                    result = 0;
                                }
                                _dbg("!")
                                VkGraphicsPipelineCreateInfo pipelineInfo = {
                                    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, NULL, 0,
                                    2, sinfo, &vertexInfo, &inputAssembly, NULL, 
                                    &viewportState, &rasterizer, &multisampling, NULL,
                                    &colorBlending, NULL, pipelineLayout, renderPass, 0,
                                    VK_NULL_HANDLE, -1};
                                vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
                                vkFb.resize(scImageViews.size());
                                for(size_t i = 0; i < vkFb.size(); ++i) {
                                   VkFramebufferCreateInfo cinfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, NULL, 0,
                                       renderPass, 1, &scImageViews[i], scExtent.width, scExtent.height, 1
                                    };
                                    vkCreateFramebuffer(vkDevice, &cinfo, NULL, &vkFb[i]);
                                }
                                _dbg("extent: %d %d", scExtent.width, scExtent.height);
                                if(vkFb[0]) {
                                    VkCommandPoolCreateInfo cinfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, NULL, 0, graphics_family};
                                    vkCreateCommandPool(vkDevice, &cinfo, NULL, &commandPool);
                                    if(commandPool) {
                                        commandBuffers.resize(vkFb.size());
                                        VkCommandBufferAllocateInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32_t)commandBuffers.size()};
                                        if(vkAllocateCommandBuffers(vkDevice, &info, commandBuffers.data()) == VK_SUCCESS) {


                                            for(size_t i = 0; i < commandBuffers.size(); ++i) {
                                                VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL};
                                                vkBeginCommandBuffer(commandBuffers[i], &info);

                                                VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
                                                VkRenderPassBeginInfo rinfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, NULL, renderPass, vkFb[i], {{0,0},scExtent}, 1, &clearColor };
                                                vkCmdBeginRenderPass(commandBuffers[i], &rinfo, VK_SUBPASS_CONTENTS_INLINE);
                                                vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
                                                VkBuffer vbuffers[] = {vertexBuffer};
                                                VkDeviceSize offsets[] = {0};
                                                vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vbuffers, offsets);
                                                vkCmdDraw(commandBuffers[i], 6, 1, 0, 0);
                                                vkCmdEndRenderPass(commandBuffers[i]);
                                                vkEndCommandBuffer(commandBuffers[i]);
                                            }

                                            result = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if(result == 0) {
            VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0};
            vkCreateSemaphore(vkDevice, &semaphoreInfo, NULL, &imageSem);
            vkCreateSemaphore(vkDevice, &semaphoreInfo, NULL, &rdoneSem);

            VkSemaphore signalSemaphores[] = {rdoneSem};
            VkSwapchainKHR swapChains[] = {vkSwapchain};

            VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, NULL, 1, signalSemaphores, 1, swapChains, NULL, NULL}; //pimageindices filled in loop

            VkSemaphore waitSemaphores[] = {imageSem};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            VkSubmitInfo submitInfo = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 1, waitSemaphores, waitStages, 1, NULL,
                1, signalSemaphores
            }; //pcommandBuffers filled in loop



            while(!done && result == 0) {
                uint32_t imageIndex = 0;
                vkAcquireNextImageKHR(vkDevice, vkSwapchain, UINT64_MAX, imageSem, VK_NULL_HANDLE, &imageIndex);
                submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
                vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
                presentInfo.pImageIndices = &imageIndex;
                vkQueuePresentKHR(presentQueue, &presentInfo);
                app.update();
                nanoGFX::nGTime::sleep(16);
            }
        }
        if(rdoneSem) vkDestroySemaphore(vkDevice, rdoneSem, NULL);
        if(imageSem) vkDestroySemaphore(vkDevice, imageSem, NULL);
        if(vertexBuffer) vkDestroyBuffer(vkDevice, vertexBuffer, NULL);
        if(vbMemory) vkFreeMemory(vkDevice, vbMemory, NULL);
        if(commandPool) vkDestroyCommandPool(vkDevice, commandPool, NULL);
        for(size_t i = 0; i < vkFb.size(); ++i) {
            vkDestroyFramebuffer(vkDevice, vkFb[i], NULL);
        }
        if(graphicsPipeline) vkDestroyPipeline(vkDevice, graphicsPipeline, nullptr);
        if(renderPass) vkDestroyRenderPass(vkDevice, renderPass, NULL);
        if(pipelineLayout) vkDestroyPipelineLayout(vkDevice, pipelineLayout, NULL);
        if(vShader) vkDestroyShaderModule(vkDevice, vShader, NULL);
        if(fShader) vkDestroyShaderModule(vkDevice, fShader, NULL);
        for(size_t i = 0; i < scImageViews.size(); ++i) {
            vkDestroyImageView(vkDevice, scImageViews[i], NULL);
        }
        if(vkSwapchain) vkDestroySwapchainKHR(vkDevice, vkSwapchain, NULL);
        vkDestroyDevice(vkDevice, NULL);
        vkDestroySurfaceKHR(vkInstance, vkSurface, NULL);
        vkDestroyInstance(vkInstance, NULL);
    }
    else {
        _dbg("VkCreateInstance failed!");
    }
    return result;
}
