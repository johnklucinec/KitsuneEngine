#include "resources.hpp"
#include "context.hpp"
#include "frame.hpp"
#include <entt/entity/fwd.hpp>

#include <ktxvulkan.h>
#include <entt/entity/registry.hpp>
#include "pipeline.hpp"
#include "utils/mesh_loader.hpp"
#include <glm/gtc/matrix_transform.hpp>


void Renderer::initSceneResources(entt::registry& registry)
{
  auto&       res = registry.ctx().get<SceneResources>();
  const auto& ctx = registry.ctx().get<VkContext>();
  const auto& fs  = registry.ctx().get<FrameState>();

  // ========================================
  // Shader Data Buffers (one per frame-in-flight)
  for(uint32_t i = 0; i < fs.framesInFlight; i++)
  {
    VkBufferCreateInfo frameDataCI{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = sizeof(ShaderData),
      .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,  // Buffer accessible via raw pointer in shader
    };
    VmaAllocationCreateInfo frameDataAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    chk(vmaCreateBuffer(ctx.allocator, &frameDataCI, &frameDataAllocCI, &res.shaderDataBuffers[i].buffer, &res.shaderDataBuffers[i].allocation,
                        &res.shaderDataBuffers[i].allocationInfo));

    // Grab and store buffer's device address
    VkBufferDeviceAddressInfo bdaInfo{
      .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = res.shaderDataBuffers[i].buffer,
    };
    res.shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(ctx.device, &bdaInfo);
  }

  // ========================================
  // Instance SSBO (dynamic, one per frame-in-flight)
  constexpr uint32_t MAX_INSTANCES = 1024;

  for(uint32_t i = 0; i < fs.framesInFlight; i++)
  {
    VkBufferCreateInfo ssboCI{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = sizeof(InstanceData) * MAX_INSTANCES,
      .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    };
    VmaAllocationCreateInfo ssboAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    chk(vmaCreateBuffer(ctx.allocator, &ssboCI, &ssboAllocCI, &res.instanceBuffers[i].buffer, &res.instanceBuffers[i].allocation, &res.instanceBuffers[i].allocationInfo));

    VkBufferDeviceAddressInfo bdaInfo{
      .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
      .buffer = res.instanceBuffers[i].buffer,
    };
    res.instanceBuffers[i].deviceAddress = vkGetBufferDeviceAddress(ctx.device, &bdaInfo);
  }

}

void Renderer::destroySceneResources(entt::registry& registry)
{
  auto&       res = registry.ctx().get<SceneResources>();
  const auto& ctx = registry.ctx().get<VkContext>();

  for(auto& tex : res.textures)
  {
    vkDestroySampler(ctx.device, tex.sampler, nullptr);
    vkDestroyImageView(ctx.device, tex.view, nullptr);
    vmaDestroyImage(ctx.allocator, tex.image, tex.allocation);
  }

  for(auto& sdb : res.shaderDataBuffers)
    vmaDestroyBuffer(ctx.allocator, sdb.buffer, sdb.allocation);

  for(auto& sdb : res.instanceBuffers)
    vmaDestroyBuffer(ctx.allocator, sdb.buffer, sdb.allocation);

  for(auto& mr : res.meshes)
    vmaDestroyBuffer(ctx.allocator, mr.buffer, mr.allocation);
}

// Updates the descriptor sets with the latest texture descriptors
void Renderer::updateDescriptorSets(entt::registry& registry)
{
  const auto& res = registry.ctx().get<SceneResources>();
  const auto& ps  = registry.ctx().get<PipelineState>();
  const auto& ctx = registry.ctx().get<VkContext>();

  if(ps.descSet == VK_NULL_HANDLE)
  {
    Log::error("ERROR", "PipelineState descSet is VK_NULL_HANDLE.");
    return;
  }

  VkWriteDescriptorSet writeDescSet{
    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet          = ps.descSet,
    .dstBinding      = 0,
    .descriptorCount = static_cast<uint32_t>(res.textureDescriptors.size()),
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo      = res.textureDescriptors.data(),
  };
  vkUpdateDescriptorSets(ctx.device, 1, &writeDescSet, 0, nullptr);
}

uint32_t Renderer::loadMesh(const std::string& path, entt::registry& registry)
{

  auto&       res = registry.ctx().get<SceneResources>();
  const auto& ctx = registry.ctx().get<VkContext>();

  // ========================================
  // Cleanup if mesh loading fails
  auto cleanup = [&]() {
    for(auto& sdb : res.shaderDataBuffers)
      if(sdb.buffer)
        vmaDestroyBuffer(ctx.allocator, sdb.buffer, sdb.allocation);
    for(auto& ib : res.instanceBuffers)
      if(ib.buffer)
        vmaDestroyBuffer(ctx.allocator, ib.buffer, ib.allocation);
  };

  // ========================================
  // Load Mesh
  auto mesh = MeshLoader::load(path.c_str());
  if(!mesh)
  {
    cleanup();
    return UINT32_MAX;
  }

  MeshResource mr{};
  VkDeviceSize vSize = sizeof(Vertex) * mesh->vertices.size();
  VkDeviceSize iSize = sizeof(uint16_t) * mesh->indices.size();
  mr.indexOffset     = vSize;
  mr.indexCount      = static_cast<uint32_t>(mesh->indices.size());

  VkBufferCreateInfo bufCI{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = vSize + iSize,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
  };

  // Persistent Mapping
  VmaAllocationCreateInfo allocCI{
    // NOTE: without ReBAR, host-visible VRAM is limited to 256MB. Exhausting it falls back to non-host-visible DEVICE_LOCAL via ALLOW_TRANSFER_INSTEAD
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
             | VMA_ALLOCATION_CREATE_MAPPED_BIT,  // fallback fires once the 256MB BAR is exhausted, which is common on non-ReBAR discrete GPUs
    .usage = VMA_MEMORY_USAGE_AUTO,
  };

  VmaAllocationInfo allocInfo{};
  chk(vmaCreateBuffer(ctx.allocator, &bufCI, &allocCI, &mr.buffer, &mr.allocation, &allocInfo));

  memcpy(allocInfo.pMappedData, mesh->vertices.data(), vSize);
  memcpy(static_cast<char*>(allocInfo.pMappedData) + vSize, mesh->indices.data(), iSize);

  uint32_t handle = static_cast<uint32_t>(res.meshes.size());
  res.meshes.push_back(mr);
  return handle;
}

uint32_t Renderer::loadTexture(const std::string& path, entt::registry& registry)
{
  auto&       res = registry.ctx().get<SceneResources>();
  const auto& ctx = registry.ctx().get<VkContext>();

  Texture tex{};

  ktxTexture* ktxTex{ nullptr };
  chk(ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex));

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
  chk(vmaCreateImage(ctx.allocator, &texImgCI, &texImageAllocCI, &tex.image, &tex.allocation, nullptr));

  // Create image view
  VkImageViewCreateInfo texViewCI{
    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image            = tex.image,
    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
    .format           = texImgCI.format,
    .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTex->numLevels, .layerCount = 1 }
  };
  chk(vkCreateImageView(ctx.device, &texViewCI, nullptr, &tex.view));


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
    .image            = tex.image,
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
  vkCmdCopyBufferToImage(cb, stagingBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

  // Transition mip levels TRANSFER_DST_OPTIMAL → READ_ONLY_OPTIMAL
  VkImageMemoryBarrier2 barrierToRead{
    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
    .srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    .dstStageMask     = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    .dstAccessMask    = VK_ACCESS_2_SHADER_READ_BIT,
    .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .newLayout        = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, // Yay, shaders can read
    .image            = tex.image,
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
  chk(vkCreateSampler(ctx.device, &samplerCI, nullptr, &tex.sampler));

  ktxTexture_Destroy(ktxTex);

  uint32_t handle = static_cast<uint32_t>(res.textures.size());
  if(handle >= MAX_TEXTURES)
    Log::error("ERROR", "Exceeded MAX_TEXTURES limit!");

  res.textures.push_back(tex);
  res.textureDescriptors.push_back({
      .sampler     = tex.sampler,
      .imageView   = tex.view,
      .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
  });

  return handle;  // this is the index into textureDescriptors
}
