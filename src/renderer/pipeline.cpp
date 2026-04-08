#include "pipeline.hpp"
#include "context.hpp"
#include "types.hpp"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <array>
#include <cassert>
#include "common.hpp"

namespace renderer {

void initPipeline(PipelineState& ps, const VkContext& ctx, VkFormat colorFmt, VkFormat depthFmt, std::span<const VkDescriptorImageInfo> textures)
{
  // ========================================
  // Descriptor Set Layout
  VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };

  // Only using bindings for images
  VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount  = 1,
    .pBindingFlags = &descVariableFlag,
  };
  VkDescriptorSetLayoutBinding descLayoutBindingTex{
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // Will be combining textures and samplers
    .descriptorCount = static_cast<uint32_t>(textures.size()),
    .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutCreateInfo descLayoutCI{
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext        = &descBindingFlags,
    .bindingCount = 1,
    .pBindings    = &descLayoutBindingTex,
  };
  chk(vkCreateDescriptorSetLayout(ctx.device, &descLayoutCI, nullptr, &ps.descSetLayout));  // Defines the interface


  // ========================================
  // Allocate Descriptor Pool
  VkDescriptorPoolSize poolSize{
    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = static_cast<uint32_t>(textures.size()) + 1,  // As many descriptors for images + samplers per texture + 1 for ImGui
  };
  VkDescriptorPoolCreateInfo descPoolCI{
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,  // required by ImGui
    .maxSets       = 2,                                                  // how mant descriptor sets, not descriptors + 1 for ImGui
    .poolSizeCount = 1,
    .pPoolSizes    = &poolSize,  // If wrong, allocations beyound requested counts will fail
  };
  chk(vkCreateDescriptorPool(ctx.device, &descPoolCI, nullptr, &ps.descPool));

  // ========================================
  // Descriptor Set Allocation + Update
  uint32_t variableDescCount{ static_cast<uint32_t>(textures.size()) };
  // Allocate descriptor sets from that pool
  VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
    .descriptorSetCount = 1,
    .pDescriptorCounts  = &variableDescCount,
  };
  VkDescriptorSetAllocateInfo descSetAI{
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext              = &variableDescCountAI,
    .descriptorPool     = ps.descPool,
    .descriptorSetCount = 1,
    .pSetLayouts        = &ps.descSetLayout,  // Interface to our shaders
  };
  chk(vkAllocateDescriptorSets(ctx.device, &descSetAI, &ps.descSet));

  VkWriteDescriptorSet writeDescSet{
    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet          = ps.descSet,
    .dstBinding      = 0,
    .descriptorCount = static_cast<uint32_t>(textures.size()),
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo      = textures.data(),
  };
  vkUpdateDescriptorSets(ctx.device, 1, &writeDescSet, 0, nullptr);  // Contains the actual descriptor data

  // ========================================
  // Shader Loading (Slang → SPIR-V)
  // Initialize Slang shader compiler
  Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
  slang::createGlobalSession(slangGlobalSession.writeRef());

  auto slangTargets{
    std::to_array<slang::TargetDesc>({ {
        .format  = SLANG_SPIRV,
        .profile = slangGlobalSession->findProfile("spirv_1_4"),
    } }
    )
  };
  auto slangOptions{
    std::to_array<slang::CompilerOptionEntry>({ {
        slang::CompilerOptionName::EmitSpirvDirectly,
        { slang::CompilerOptionValueKind::Int, 1 },
    } }
    )
  };

  slang::SessionDesc slangSessionDesc{
    .targets                  = slangTargets.data(),
    .targetCount              = SlangInt(slangTargets.size()),
    .defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,  // Matches the layout of GLM
    .compilerOptionEntries    = slangOptions.data(),
    .compilerOptionEntryCount = uint32_t(slangOptions.size()),
  };
  Slang::ComPtr<slang::ISession> slangSession;
  slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

  // Load the Shader
  Slang::ComPtr<slang::IModule> slangModule{ slangSession->loadModuleFromSource("triangle", "assets/shaders/shader.slang", nullptr, nullptr) };
  Slang::ComPtr<ISlangBlob>     spirv;
  slangModule->getTargetCode(0, spirv.writeRef());

  // Create shader module for use in graphics pipeline
  VkShaderModuleCreateInfo shaderModuleCI{
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = spirv->getBufferSize(),
    .pCode    = reinterpret_cast<const uint32_t*>(spirv->getBufferPointer()),
  };
  VkShaderModule shaderModule{};
  chk(vkCreateShaderModule(ctx.device, &shaderModuleCI, nullptr, &shaderModule));

  // ========================================
  // Graphics Pipeline Layout
  VkPushConstantRange pushConstantRange{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .size       = sizeof(VkDeviceAddress),
  };
  VkPipelineLayoutCreateInfo pipelineLayoutCI{
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = 1,
    .pSetLayouts            = &ps.descSetLayout,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges    = &pushConstantRange,
  };
  chk(vkCreatePipelineLayout(ctx.device, &pipelineLayoutCI, nullptr, &ps.layout));

  // ========================================
  // Graphics Pipeline
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
    { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT,   .module = shaderModule, .pName = "main" },
    { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = shaderModule, .pName = "main" },
  };
  VkVertexInputBindingDescription vertexBinding{
    .binding   = 0,
    .stride    = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  std::vector<VkVertexInputAttributeDescription> vertexAttributes{
    { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
    { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
    { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
  };

  // Setup all the pipeline states
  VkPipelineVertexInputStateCreateInfo vertexInputState{
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = 1,
    .pVertexBindingDescriptions      = &vertexBinding,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
    .pVertexAttributeDescriptions    = vertexAttributes.data(),
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
    .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // we be rendering a list of triangles
  };
  std::vector<VkDynamicState>      dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamicState{
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates    = dynamicStates.data(),
  };
  VkPipelineViewportStateCreateInfo viewportState{
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };
  VkPipelineRasterizationStateCreateInfo rasterizationState{
    .sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .lineWidth = 1.0f,  // Not used (default state)
  };
  VkPipelineMultisampleStateCreateInfo multisampleState{
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,  // Not used (default state)
  };
  VkPipelineDepthStencilStateCreateInfo depthStencilState{
    .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable  = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL,
  };
  VkPipelineColorBlendAttachmentState blendAttachment{
    .colorWriteMask = 0xF,  // Not used (default state)
  };
  VkPipelineColorBlendStateCreateInfo colorBlendState{
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &blendAttachment,
  };
  VkPipelineRenderingCreateInfo renderingCI{
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,  // We are using dynamic rendering
    .colorAttachmentCount    = 1,
    .pColorAttachmentFormats = &colorFmt,
    .depthAttachmentFormat   = depthFmt,
  };

  // Create the Graphics Pipeline
  VkGraphicsPipelineCreateInfo pipelineCI{
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext               = &renderingCI,
    .stageCount          = static_cast<uint32_t>(shaderStages.size()),
    .pStages             = shaderStages.data(),
    .pVertexInputState   = &vertexInputState,
    .pInputAssemblyState = &inputAssemblyState,
    .pViewportState      = &viewportState,
    .pRasterizationState = &rasterizationState,
    .pMultisampleState   = &multisampleState,
    .pDepthStencilState  = &depthStencilState,
    .pColorBlendState    = &colorBlendState,
    .pDynamicState       = &dynamicState,
    .layout              = ps.layout,
  };
  chk(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &ps.pipeline));

  // shaderModule is only needed during pipeline creation
  vkDestroyShaderModule(ctx.device, shaderModule, nullptr);
}

void destroyPipeline(PipelineState& ps, const VkContext& ctx)
{
  vkDestroyPipeline(ctx.device, ps.pipeline, nullptr);
  vkDestroyPipelineLayout(ctx.device, ps.layout, nullptr);
  vkDestroyDescriptorPool(ctx.device, ps.descPool, nullptr);  // implicitly frees descSet
  vkDestroyDescriptorSetLayout(ctx.device, ps.descSetLayout, nullptr);
}

}  // namespace renderer
