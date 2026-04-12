#include "resources.hpp"
#include "context.hpp"
#include "frame.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <ktxvulkan.h>
#include <cstring>
#include <string>
#include <vector>
#include "types.hpp"

#include "../assets/shaders/shader_constants.h"

void renderer::initSceneResources(SceneResources& res, const VkContext& ctx, const FrameState& fs)
{
  // ========================================
  // Load Mesh
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/models/suzanne.obj"));

  std::vector<Vertex>   vertices;
  std::vector<uint16_t> indices;

  // Load vertex and index data
  for(auto& index : shapes[0].mesh.indices)
  {
    Vertex v{
      .pos    = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
      .normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
      .uv     = { attrib.texcoords[index.texcoord_index * 2], 1.0f - attrib.texcoords[index.texcoord_index * 2 + 1] }
    };
    vertices.push_back(v);
    indices.push_back(static_cast<uint16_t>(indices.size()));
  }

  res.indexCount = static_cast<uint32_t>(indices.size());

  // ========================================
  // Allocate vertex + index buffer
  VkDeviceSize       vBufSize{ sizeof(Vertex) * vertices.size() };
  VkDeviceSize       iBufSize{ sizeof(uint16_t) * indices.size() };
  VkBufferCreateInfo bufferCI{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = vBufSize + iBufSize,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
  };

  res.indexOffset = vBufSize;

  // Persistent Mapping
  VmaAllocationCreateInfo vBufferAllocCI{
    // NOTE: without ReBAR, host-visible VRAM is limited to 256MB. Exhausting it falls back to non-host-visible DEVICE_LOCAL via ALLOW_TRANSFER_INSTEAD
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
             | VMA_ALLOCATION_CREATE_MAPPED_BIT,  // fallback fires once the 256MB BAR is exhausted, which is common on non-ReBAR discrete GPUs
    .usage = VMA_MEMORY_USAGE_AUTO,
  };

  VmaAllocationInfo vBufferAllocInfo{};
  chk(vmaCreateBuffer(ctx.allocator, &bufferCI, &vBufferAllocCI, &res.vBuffer, &res.vBufferAlloc, &vBufferAllocInfo));

  // Write directly into VRAM via persistently mapped ReBAR memory
  memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
  memcpy(static_cast<char*>(vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);


  // ========================================
  // Shader Data Buffers (one per frame-in-flight)
  for(uint32_t i = 0; i < fs.framesInFlight; i++)
  {
    VkBufferCreateInfo uBufferCI{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = sizeof(ShaderData),
      .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // Buffer accessible via raw pointer in shader
    };
    VmaAllocationCreateInfo uBufferAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT,  // Persistently mapped; CPU/GPU accessible
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    chk(vmaCreateBuffer(ctx.allocator, &uBufferCI, &uBufferAllocCI, &res.shaderDataBuffers[i].buffer, &res.shaderDataBuffers[i].allocation,
                        &res.shaderDataBuffers[i].allocationInfo));

    // Grab and store buffer's device address
    VkBufferDeviceAddressInfo uBufferBdaInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = res.shaderDataBuffers[i].buffer };
    res.shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(ctx.device, &uBufferBdaInfo);
  }


  // ========================================
  // Load Textures
  std::vector<VkDescriptorImageInfo> textureDescriptors;
  for(int i = 0; i < static_cast<int>(res.textures.size()); i++)
  {
    ktxTexture* ktxTex{ nullptr };
    std::string filename = "assets/models/suzanne" + std::to_string(i % 3) + ".ktx";  //TODO: Remove % 3 when I dont use INSTANCE_COUNT
    chk(ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex));

    // Create image
    VkImageCreateInfo texImgCI{
      .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType     = VK_IMAGE_TYPE_2D,
      .format        = ktxTexture_GetVkFormat(ktxTex),
      .extent        = { .width = ktxTex->baseWidth, .height = ktxTex->baseHeight, .depth = 1 },
      .mipLevels     = ktxTex->numLevels,
      .arrayLayers   = 1,
      .samples       = VK_SAMPLE_COUNT_1_BIT,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
    chk(vmaCreateImage(ctx.allocator, &texImgCI, &texImageAllocCI, &res.textures[i].image, &res.textures[i].allocation, nullptr));

    // Create image view
    VkImageViewCreateInfo texViewCI{
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = res.textures[i].image,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .format           = texImgCI.format,
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTex->numLevels, .layerCount = 1 }
    };
    chk(vkCreateImageView(ctx.device, &texViewCI, nullptr, &res.textures[i].view));


    // ========================================
    // Upload
    // Staging buffer
    VkBuffer           stagingBuffer{};
    VmaAllocation      stagingAllocation{};
    VkBufferCreateInfo stagingCI{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = ktxTex->dataSize,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  // Temporary source for a buffer-to-image copy
    };
    VmaAllocationCreateInfo stagingAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VmaAllocationInfo stagingAllocInfo{};
    chk(vmaCreateBuffer(ctx.allocator, &stagingCI, &stagingAllocCI, &stagingBuffer, &stagingAllocation, &stagingAllocInfo));
    memcpy(stagingAllocInfo.pMappedData, ktxTex->pData, ktxTex->dataSize);  // Actually move image data now

    // Copy image data to the optimal tiled image on the GPU (One-time command buffer for upload)
    VkFence           fence{};
    VkFenceCreateInfo fenceCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    chk(vkCreateFence(ctx.device, &fenceCI, nullptr, &fence));

    VkCommandBuffer             cb{};
    VkCommandBufferAllocateInfo cbAI{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = ctx.commandPool,
      .commandBufferCount = 1,
    };
    chk(vkAllocateCommandBuffers(ctx.device, &cbAI, &cb));

    // Start recording the commands required to get the image to its destination
    VkCommandBufferBeginInfo cbBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));

    // Transition mip level UNDEFINED → TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier2 barrierToTransfer{
      .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
      .srcAccessMask    = VK_ACCESS_2_NONE,
      .dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      .dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
      .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // The good layout
      .image            = res.textures[i].image,
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTex->numLevels, .layerCount = 1 }
    };
    VkDependencyInfo depInfo{
      .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers    = &barrierToTransfer,
    };
    vkCmdPipelineBarrier2(cb, &depInfo);

    // Copy all mip levels from staging buffer
    std::vector<VkBufferImageCopy> copyRegions;
    for(uint32_t j = 0; j < ktxTex->numLevels; j++)
    {
      ktx_size_t mipOffset{ 0 };
      ktxTexture_GetImageOffset(ktxTex, j, 0, 0, &mipOffset);
      copyRegions.push_back({
          .bufferOffset     = mipOffset,
          .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = j,                     .layerCount = 1 },
          .imageExtent      = { .width = ktxTex->baseWidth >> j,         .height = ktxTex->baseHeight >> j, .depth = 1      },
      });
    }

    // Copy mip levels from temp buffer
    vkCmdCopyBufferToImage(cb, stagingBuffer, res.textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()),
                           copyRegions.data());

    // Transition mip levels TRANSFER_DST_OPTIMAL → READ_ONLY_OPTIMAL
    VkImageMemoryBarrier2 barrierToRead{
      .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      .srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
      .dstStageMask     = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      .dstAccessMask    = VK_ACCESS_2_SHADER_READ_BIT,
      .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout        = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, // Yay, shaders can read
      .image            = res.textures[i].image,
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTex->numLevels, .layerCount = 1 }
    };
    depInfo.pImageMemoryBarriers = &barrierToRead;
    vkCmdPipelineBarrier2(cb, &depInfo);

    chk(vkEndCommandBuffer(cb));
    VkCommandBufferSubmitInfo commandBufferInfo{
      .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = cb,
    };
    VkSubmitInfo2 submitInfo{
      .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .commandBufferInfoCount = 1,
      .pCommandBufferInfos    = &commandBufferInfo,
    };
    chk(vkQueueSubmit2(ctx.queue, 1, &submitInfo, fence));
    chk(vkWaitForFences(ctx.device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(ctx.device, fence, nullptr);
    vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &cb);
    vmaDestroyBuffer(ctx.allocator, stagingBuffer, stagingAllocation);


    // ========================================
    // Create Sample Texture in Shader
    VkSamplerCreateInfo samplerCI{
      .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter        = VK_FILTER_LINEAR,
      .minFilter        = VK_FILTER_LINEAR,
      .mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy    = 8.0f,  // 8 is a widely supported value for max anisotropy
      .maxLod           = static_cast<float>(ktxTex->numLevels),
    };
    chk(vkCreateSampler(ctx.device, &samplerCI, nullptr, &res.textures[i].sampler));

    // Clean up and store descriptor info for texture to use later
    ktxTexture_Destroy(ktxTex);
    textureDescriptors.push_back({
        .sampler     = res.textures[i].sampler,
        .imageView   = res.textures[i].view,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
    });
  }

  res.textureDescriptors = textureDescriptors;
}

void renderer::destroySceneResources(SceneResources& res, const VkContext& ctx)
{
  for(auto& tex : res.textures)
  {
    vkDestroySampler(ctx.device, tex.sampler, nullptr);
    vkDestroyImageView(ctx.device, tex.view, nullptr);
    vmaDestroyImage(ctx.allocator, tex.image, tex.allocation);
  }
  for(auto& sdb : res.shaderDataBuffers)
    vmaDestroyBuffer(ctx.allocator, sdb.buffer, sdb.allocation);

  vmaDestroyBuffer(ctx.allocator, res.vBuffer, res.vBufferAlloc);
}
