/*
 * Photon Core
 *
 */

#include "VulkanglTFModel.h"
#include "gui.hpp"
#include "vulkanexamplebase.h"
#include <imgui.h>

VkSampler sampler;

// ----------------------------------------------------------------------------
// Photon
// ----------------------------------------------------------------------------

class Photon : public VulkanExampleBase {
public:
  GUI *gui = nullptr;

  struct Models {
    vkglTF::Model models;
    vkglTF::Model logos;
    vkglTF::Model background;
    vkglTF::Model customModel; // new
  } models;

  vks::Buffer uniformBufferVS;

  struct UBOVS {
    glm::mat4 projection;
    glm::mat4 modelview;
    glm::vec4 lightPos;
  } uboVS;

  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  VkPipeline customPipeline; // custom
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;

  Photon() : VulkanExampleBase() {
    title = "Photon";
    camera.type = Camera::CameraType::lookat;
    camera.setPosition(glm::vec3(0.0f, 0.0f, -4.8f));
    camera.setRotation(glm::vec3(4.5f, -380.0f, 0.0f));
    camera.setPerspective(45.0f, (float)width / (float)height, 0.1f, 256.0f);

    // SRS - Enable VK_KHR_get_physical_device_properties2 to retrieve device
    // driver information for display
    enabledInstanceExtensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // Don't use the ImGui overlay of the base framework in this sample
    settings.overlay = false;
  }

  ~Photon() {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipeline(device, customPipeline, nullptr); // custom
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    uniformBufferVS.destroy();

    delete gui;
  }

  void buildCommandBuffers() {
    // I lied ... it happens here :)
    VkCommandBufferBeginInfo cmdBufInfo =
        vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = {{0.2f, 0.2f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo =
        vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    gui->newFrame(this, (frameCounter == 0));
    gui->updateBuffers();

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
      // Set target frame buffer
      renderPassBeginInfo.framebuffer = frameBuffers[i];

      VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

      vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport =
          vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
      vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

      VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
      vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

      // Render scene
      vkCmdBindDescriptorSets(drawCmdBuffers[i],
                              VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                              0, 1, &descriptorSet, 0, nullptr);
      vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline);

      VkDeviceSize offsets[1] = {0};
      if (uiSettings.displayBackground) {
        models.background.draw(drawCmdBuffers[i]);
      }

      if (uiSettings.displayModels) {
        models.models.draw(drawCmdBuffers[i]);
      }

      if (uiSettings.displayLogos) {
        models.logos.draw(drawCmdBuffers[i]);
      }

      if (uiSettings.displayCustomModel) { // imgui to toggle visibility
        models.customModel.draw(drawCmdBuffers[i]);
      }

      // Render imGui
      if (ui.visible) {
        gui->drawFrame(drawCmdBuffers[i]);
      }

      vkCmdEndRenderPass(drawCmdBuffers[i]);

      VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
    }
  }

  void setupLayoutsAndDescriptors() {
    // descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              2),
        vks::initializers::descriptorPoolSize(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr,
                                           &descriptorPool));

    // Set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout,
                                                nullptr, &descriptorSetLayout));

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                           nullptr, &pipelineLayout));

    // Descriptor set
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool,
                                                     &descriptorSetLayout, 1);
    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(descriptorSet,
                                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              0, &uniformBufferVS.descriptor),
    };
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writeDescriptorSets.size()),
                           writeDescriptorSets.data(), 0, nullptr);
  }

  void preparePipelines() {
    // Rendering
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
            VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            VK_SAMPLE_COUNT_1_BIT);
    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCI =
        vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();
    pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal,
         vkglTF::VertexComponent::Color});
    ;

    shaderStages[0] = loadShader(getShadersPath() + "imgui/scene.vert.spv",
                                 VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getShadersPath() + "imgui/scene.frag.spv",
                                 VK_SHADER_STAGE_FRAGMENT_BIT);
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1,
                                              &pipelineCI, nullptr, &pipeline));

    // custom pipeline stuff
    std::array<VkPipelineShaderStageCreateInfo, 2> customShaderStages;

    VkGraphicsPipelineCreateInfo customPipelineCI =
        vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
    customPipelineCI.pInputAssemblyState = &inputAssemblyState;
    customPipelineCI.pRasterizationState = &rasterizationState;
    customPipelineCI.pColorBlendState = &colorBlendState;
    customPipelineCI.pMultisampleState = &multisampleState;
    customPipelineCI.pViewportState = &viewportState;
    customPipelineCI.pDepthStencilState = &depthStencilState;
    customPipelineCI.pDynamicState = &dynamicState;
    customPipelineCI.stageCount =
        static_cast<uint32_t>(customShaderStages.size());
    customPipelineCI.pStages = customShaderStages.data();
    customPipelineCI.pVertexInputState =
        vkglTF::Vertex::getPipelineVertexInputState(
            {vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal,
             vkglTF::VertexComponent::Color});

    customShaderStages[0] =
        loadShader(getShadersPath() + "custom/custom_model.vert.spv",
                   VK_SHADER_STAGE_VERTEX_BIT);
    customShaderStages[1] =
        loadShader(getShadersPath() + "custom/custom_model.frag.spv",
                   VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        device, pipelineCache, 1, &customPipelineCI, nullptr, &customPipeline));
  }

  // Prepare and initialize uniform buffer containing shader uniforms
  void prepareUniformBuffers() {
    // Vertex shader uniform buffer block
    VK_CHECK_RESULT(
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uniformBufferVS, sizeof(uboVS), &uboVS));

    updateUniformBuffers();
  }

  void updateUniformBuffers() {
    // Vertex shader
    uboVS.projection = camera.matrices.perspective;
    uboVS.modelview = camera.matrices.view * glm::mat4(1.0f);

    // Light source
    if (uiSettings.animateLight) {
      uiSettings.lightTimer += frameTimer * uiSettings.lightSpeed;
      uboVS.lightPos.x =
          sin(glm::radians(uiSettings.lightTimer * 360.0f)) * 15.0f;
      uboVS.lightPos.z =
          cos(glm::radians(uiSettings.lightTimer * 360.0f)) * 15.0f;
    };

    VK_CHECK_RESULT(uniformBufferVS.map());
    memcpy(uniformBufferVS.mapped, &uboVS, sizeof(uboVS));
    uniformBufferVS.unmap();
  }

  void draw() {
    VulkanExampleBase::prepareFrame();
    buildCommandBuffers();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VulkanExampleBase::submitFrame();
  }

  void loadAssets() {
    const uint32_t glTFLoadingFlags =
        vkglTF::FileLoadingFlags::PreTransformVertices |
        vkglTF::FileLoadingFlags::PreMultiplyVertexColors |
        vkglTF::FileLoadingFlags::FlipY;
    models.models.loadFromFile(getAssetPath() + "models/vulkanscenemodels.gltf",
                               vulkanDevice, queue, glTFLoadingFlags);
    models.background.loadFromFile(getAssetPath() +
                                       "models/vulkanscenebackground.gltf",
                                   vulkanDevice, queue, glTFLoadingFlags);
    models.logos.loadFromFile(getAssetPath() + "models/vulkanscenelogos.gltf",
                              vulkanDevice, queue, glTFLoadingFlags);
    models.customModel.loadFromFile(getAssetPath() + "models/custom_model.gltf",
                                    vulkanDevice, queue, glTFLoadingFlags);
  }

  void prepareImGui() {
    gui = new GUI(this);
    gui->init((float)width, (float)height);
    gui->initResources(renderPass, queue, getShadersPath());
  }

  void prepare() {
    VulkanExampleBase::prepare();
    loadAssets();
    prepareUniformBuffers();
    setupLayoutsAndDescriptors();
    preparePipelines();
    prepareImGui();
    buildCommandBuffers();
    prepared = true;
  }

  virtual void render() { // where the magic happens
    if (!prepared)
      return;

    updateUniformBuffers();

    // ImGui input/output
    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DeltaTime = frameTimer;

    io.MousePos = ImVec2(mouseState.position.x, mouseState.position.y);
    io.MouseDown[0] = mouseState.buttons.left && ui.visible;
    io.MouseDown[1] = mouseState.buttons.right && ui.visible;
    io.MouseDown[2] = mouseState.buttons.middle && ui.visible;

    draw();
  }

  virtual void mouseMoved(double x, double y, bool &handled) {
    ImGuiIO &io = ImGui::GetIO();
    handled = io.WantCaptureMouse && ui.visible;
  }

// Input handling is platform specific, to show how it's basically done this
// sample implements it for Windows
#if defined(_WIN32)
  virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam,
                               LPARAM lParam) {
    ImGuiIO &io = ImGui::GetIO();
    // Only react to keyboard input if ImGui is active
    if (io.WantCaptureKeyboard) {
      // Character input
      if (uMsg == WM_CHAR) {
        if (wParam > 0 && wParam < 0x10000) {
          io.AddInputCharacter((unsigned short)wParam);
        }
      }
      // Special keys (tab, cursor, etc.)
      if ((wParam < 256) && (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)) {
        io.KeysDown[wParam] = true;
      }
      if ((wParam < 256) && (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP)) {
        io.KeysDown[wParam] = false;
      }
    }
  }
#endif
};

// Main Entrypoints
PHOTON_MAIN()
