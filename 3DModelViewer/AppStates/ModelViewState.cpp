#include "ModelViewState.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <XGREngine/Application.h>

#include <XGR/helper/Utils.h>
#include <XGR/helper/vk_initializers.h>

#include <vulkan/vk_enum_string_helper.h>

#include "../AppDefinitions.h"
#include <filesystem>
#include <XGR/cmd/XFastSubmit.h>

using namespace states;
using namespace XGREngine;

namespace ImGui {
	void IndeterminateProgressBar(const ImVec2& size_arg)
	{
		using namespace ImGui;

		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiStyle& style = g.Style;
		ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
		ImVec2 pos = window->DC.CursorPos;
		ImRect bb(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
		ItemSize(size);
		if (!ItemAdd(bb, 0))
			return;

		const float speed = g.FontSize * 0.05f;
		const float phase = ImFmod((float)g.Time * speed, 1.0f);
		const float width_normalized = 0.2f;
		float t0 = phase * (1.0f + width_normalized) - width_normalized;
		float t1 = t0 + width_normalized;

		RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
		bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
		RenderRectFilledRangeH(window->DrawList, bb, GetColorU32(ImGuiCol_PlotHistogram), t0, t1, style.FrameRounding);
	}
}


VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void states::ModelViewState::createRenderTargetPipeline()
{
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &_data->colorSwapchainFormat;
	renderingInfo.depthAttachmentFormat = _data->depthSwapchainFormat;


	xgr::XGraphicPipelineDesc pipelineDesc{};
	pipelineDesc.renderPass = nullptr;
	pipelineDesc.pNext = &renderingInfo;
	pipelineDesc.pipelineLayout = this->target0.drawPipelineLayout.get();
	pipelineDesc.dynamicState = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	pipelineDesc.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineDesc.primitiveRestart = VK_FALSE;
	pipelineDesc.viewport = { viewport };
	pipelineDesc.scissor = { scissor };

	pipelineDesc.shaderStages = {
		{ VK_SHADER_STAGE_FRAGMENT_BIT, xgr::xgr_readFileContent("Content/Shaders/Render_RT0.spv") },
		{ VK_SHADER_STAGE_VERTEX_BIT, xgr::xgr_readFileContent("Content/Shaders/Video_ScreenTri.spv") }
	};
	this->target0.drawPipeline = xgr::XPipeline(_data->device, &pipelineDesc);

}

void states::ModelViewState::createPipelines()
{
	VkFormat colorFormat = target0.renderTarget.getColorFormat();
	VkFormat depthFormat = target0.renderTarget.getDepthFormat();

	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachmentFormats = &colorFormat;
	renderingInfo.depthAttachmentFormat = depthFormat;

	xgr::XGraphicPipelineDesc pipelineDesc{};

	pipelineDesc.dynamicState = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	pipelineDesc.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineDesc.primitiveRestart = VK_FALSE;
	pipelineDesc.viewport = { viewport };
	pipelineDesc.scissor = { scissor };
	pipelineDesc.shaderStages = {
		{ VK_SHADER_STAGE_FRAGMENT_BIT, xgr::xgr_readFileContent("Content/Shaders/basic_model.spv") },
		{ VK_SHADER_STAGE_VERTEX_BIT, xgr::xgr_readFileContent("Content/Shaders/model_vs.spv") }
	};

	for (size_t i = 0; i < xgr::XVertex::getVertexInputDesc().size(); i++)
		pipelineDesc.vertexInput.attributeDesc.push_back(xgr::XVertex::getVertexInputDesc().at(i));

	pipelineDesc.vertexInput.bindingDesc = {
		xgr::XVertex::getBindingDescription()
	};

	pipelineDesc.multiSample = {
		.sampleShadingRating = false,
		.minSampleShading = 1.0f,
		.rasterizationSamples = msaa_sample_count,
	};

	pipelineDesc.pipelineLayout = pipelineLayout.get();
	pipelineDesc.pipelineCache = pipelineCache.get();
	pipelineDesc.renderPass = nullptr;
	pipelineDesc.pNext = &renderingInfo;

	pipelineDesc.rasterizer = {};
	pipelineDesc.depthStencilState = {};
	pipelineDesc.blendState = {};
	this->generalPipeline = xgr::XPipeline(_data->device, &pipelineDesc);
	xgr::XDebugUtils::Get().setObjectName(_data->device, (uint64_t)generalPipeline.get(), VK_OBJECT_TYPE_PIPELINE, "ModelViewer_generalPipeline");
}

void states::ModelViewState::destroyPipelines()
{
	this->generalPipeline.destroy();
}

void states::ModelViewState::loadModel(std::string modelName)
{
	if (isModelLoaded.load()) {
		this->object_model->destroy();
		delete object_model;
	}

	isModelLoaded.store(false);

	renderer::ModelDesc model_desc{};
	model_desc.device = _data->device;
	model_desc.physicalDevice = _data->physicalDevice;
	model_desc.memoryBufferLoader = this->sceneMemory;
	model_desc.memoryImageLoader = this->imageMemory;
	model_desc.filePath = modelName;

	this->object_model = new renderer::Model(model_desc);

	for (uint32_t i = 0; i < this->object_model->getNumChilds(); i++) {
		renderer::Model::Child child = this->object_model->getChild(i);

		for (size_t j = 0; j < child.textures.size(); j++) {
			VkDescriptorImageInfo imgInfo{};
			imgInfo.sampler = nullptr;
			imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			if (child.hasDiffuseTexture) {
				imgInfo.imageView = child.textures[j]->imageView;
				this->descriptor.SetImageElement(1, 0, { imgInfo }); // <- diffuse
			}

			if (child.hasSpecularTexture) {
				imgInfo.imageView = child.textures[j + 1]->imageView;
				this->descriptor.SetImageElement(2, 0, { imgInfo }); // <- specular
			}

			if (child.hasNormalTexture) {
				imgInfo.imageView = child.textures[j + 2]->imageView;
				this->descriptor.SetImageElement(3, 0, { imgInfo }); // <- normal
			}
		}
	}

	isModelLoaded.store(true);
}

void ModelViewState::init() {
	this->loadingThread = std::make_unique<XGREngine::Thread>();

	this->options_file = XGREngine::ConfigFile(APP_CONTENT_DIRECTORY + "GraphicSettings.cfg");

	this->viewport.x = 0;
	this->viewport.y = 0;
	this->viewport.minDepth = 0.0f;
	this->viewport.maxDepth = 1.0f;
	this->viewport.width = static_cast<float>(_data->window->GetWidth());
	this->viewport.height = static_cast<float>(_data->window->GetHeight());

	this->scissor.offset = { 0, 0 };
	this->scissor.extent = { static_cast<uint32_t>(this->viewport.width), static_cast<uint32_t>(this->viewport.height) };

	this->msaa_sample_count = VK_SAMPLE_COUNT_1_BIT;
	this->last_msaa_sample_count = this->msaa_sample_count;

	// Alloc memory
	this->internalMemory = new memory::BufferMemory(VKCOLO_MEM_MiB(2), VK_SHARING_MODE_EXCLUSIVE, _data->device, _data->physicalDevice);
	this->sceneMemory = new memory::BufferMemory(VKCOLO_MEM_MiB(500), VK_SHARING_MODE_EXCLUSIVE, _data->device, _data->physicalDevice);
	this->imageMemory = new memory::ImageMemory(VKCOLO_MEM_MiB(100), _data->device, _data->physicalDevice);

	// Pipeline Layout creation
	// - Descriptor Pool
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, AppSettings::MAX_BINDLESS },
		{ VK_DESCRIPTOR_TYPE_SAMPLER, AppSettings::MAX_SAMPLERS }
	};

	this->descriptorPool = xgr::XDescriptorPool(_data->device, poolSizes, _data->framesInFlight, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

	// - Descriptor
	std::vector<memory::DescriptorObject> bindings = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, {}, false},

		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10, VK_SHADER_STAGE_FRAGMENT_BIT, {}, true},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10, VK_SHADER_STAGE_FRAGMENT_BIT, {}, true},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10, VK_SHADER_STAGE_FRAGMENT_BIT, {}, true},
		{VK_DESCRIPTOR_TYPE_SAMPLER, AppSettings::MAX_SAMPLERS, VK_SHADER_STAGE_FRAGMENT_BIT, {}, false},
	};

	this->descriptor = memory::Descriptor(_data->device, descriptorPool.get(), _data->framesInFlight, bindings);

	// - Layout

	xgr::XPipelineLayoutDesc layoutDesc{};
	for (int i = 0; i < _data->framesInFlight; i++)
		layoutDesc.descriptorLayouts.push_back(this->descriptor.getLayout(i));

	const uint32_t offset = static_cast<uint32_t>(xgr::aligned_size(sizeof(GPUDrawData), 16));

	layoutDesc.constants = {
		{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawData) },
		{ VK_SHADER_STAGE_FRAGMENT_BIT, offset, sizeof(GPUDrawColorData) }
	};

	this->pipelineLayout = xgr::XPipelineLayout(_data->device, layoutDesc);


	// Pipeline Cache creation	
	this->pipelineCache = xgr::XPipelineCache(_data->device, APP_CONTENT_DIRECTORY + "pipeline_scene.cache");


	// Samplers creation
	{
		xgr::XSamplerDesc samplerDesc{};
		samplerDesc.addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.magFilter = VK_FILTER_LINEAR;
		samplerDesc.minFilter = VK_FILTER_LINEAR;

		linearSampler = xgr::XSampler(_data->device, samplerDesc);

		samplerDesc.addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerDesc.magFilter = VK_FILTER_NEAREST;
		samplerDesc.minFilter = VK_FILTER_NEAREST;

		pointSampler = xgr::XSampler(_data->device, samplerDesc);

		VkDescriptorImageInfo linearInfo{}, pointInfo{};
		linearInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		linearInfo.imageView = nullptr;
		linearInfo.sampler = linearSampler.get();

		pointInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		pointInfo.imageView = nullptr;
		pointInfo.sampler = pointSampler.get();


		this->descriptor.SetImageElement(4, 0, { linearInfo, pointInfo });
	}

	// Create buffers
	// - get physical device properties
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(_data->physicalDevice, &properties);

	this->m_cameraUBO.resize(_data->framesInFlight);
	this->m_cameraData.resize(_data->framesInFlight);
	this->m_lightSSBO.resize(_data->framesInFlight);
	this->m_lightData.resize(AppSettings::MAX_LIGHTS_COUNT);

	for (auto& buffer : this->m_cameraUBO)
		buffer = this->internalMemory->CreateBuffer(sizeof(CameraMatrix), nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


	uint64_t ssboSize = xgr::aligned_size(AppSettings::MAX_LIGHTS_COUNT * sizeof(Light), properties.limits.minStorageBufferOffsetAlignment);
	for (auto& buffer : this->m_lightSSBO)
		buffer = this->internalMemory->CreateBuffer(ssboSize, nullptr, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	for (auto& data : this->m_lightData)
		data = fxCreateNullLight();

	// - add buffers to descriptor
	for (uint32_t i = 0; i < _data->framesInFlight; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = this->m_cameraUBO[i]->buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraMatrix);

		this->descriptor.SetBufferElement(0, 0, { bufferInfo });

		//bufferInfo.buffer = this->m_lightSSBO[i]->buffer;
		//bufferInfo.offset = 0;
		//bufferInfo.range = this->m_lightSSBO[i]->size;

		//this->descriptor.SetBufferElement(1, 0, { bufferInfo });
	}


	// Create RenderTarget
	std::vector<VkDescriptorPoolSize> target_poolSize = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};
	this->target0.descriptorPool = xgr::XDescriptorPool(_data->device, target_poolSize, _data->framesInFlight, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

	std::vector<XGREngine::memory::DescriptorObject> target_bindings = {
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, {}, true}
	};
	this->target0.drawDescriptor = memory::Descriptor(_data->device, this->target0.descriptorPool.get(), _data->framesInFlight, target_bindings);

	xgr::XPipelineLayoutDesc target_layoutDesc{};
	for (int i = 0; i < static_cast<int>(_data->framesInFlight); i++) target_layoutDesc.descriptorLayouts.push_back(this->target0.drawDescriptor.getLayout(i));
	this->target0.drawPipelineLayout = xgr::XPipelineLayout(_data->device, target_layoutDesc);

	this->createRenderTargetPipeline();

	renderer::RenderTargetDesc targetDesc{};
	targetDesc.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	targetDesc.depthFormat = VK_FORMAT_D32_SFLOAT;
	targetDesc.width = 1440;
	targetDesc.height = 900;
	targetDesc.sampleCount = msaa_sample_count;
	this->target0.renderTarget = renderer::RenderTarget(_data->device, _data->physicalDevice, targetDesc);

	this->target0.drawDescriptor.SetImageElement(0, 0, { this->target0.renderTarget.getDescriptorInfo(pointSampler.get()) });

	this->loadingThread->addJob([=]() { this->createPipelines(); });

	// Pre load a mesh
	this->loadingThread->addJob([=]() { this->loadModel("Content/dragon/dragon.obj"); });

	this->m_lightData[0] = fxCreatePointLight(vec4(1.0f, 0.0f, 0.0f, 50.0f), vec3(50.0f, 0.0f, 0.0f));
	this->m_lightData[1] = fxCreatePointLight(vec4(0.0f, 1.0f, 0.0f, 50.0f), vec3(0.0f, 50.0f, 0.0f));
	this->m_lightData[2] = fxCreatePointLight(vec4(0.0f, 0.0f, 1.0f, 50.0f), vec3(0.0f, 0.0f, 50.0f));
}

void ModelViewState::update(float deltatime) {

	// Update input
	if (this->useFreeCamera) {
		if (_data->window->IsKeyPressed('W')) {
			this->camera.ProcessKeyboard(renderer::Camera_Movement::FORWARD, deltatime);
		}
		if (_data->window->IsKeyPressed('S')) {
			this->camera.ProcessKeyboard(renderer::Camera_Movement::BACKWARD, deltatime);
		}
		if (_data->window->IsKeyPressed('A')) {
			this->camera.ProcessKeyboard(renderer::Camera_Movement::RIGHT, deltatime);
		}
		if (_data->window->IsKeyPressed('D')) {
			this->camera.ProcessKeyboard(renderer::Camera_Movement::LEFT, deltatime);
		}

		static float lastMouseX = 0, lastMouseY = 0;
		static bool firstClick = true;

		if (_data->window->IsMouseButtonPressed(xgr::MouseButton::Right))
		{
			_data->window->GrabCursor(true);
			int mouseX, mouseY;
			_data->window->GetMousePosition(mouseX, mouseY);

			if (firstClick)
			{
				lastMouseX = (float)mouseX;
				lastMouseY = (float)mouseY;
				firstClick = false;
			}

			float xoffset = lastMouseX - (float)mouseX;
			float yoffset = lastMouseY - (float)mouseY;

			lastMouseX = (float)mouseX;
			lastMouseY = (float)mouseY;

			this->camera.ProcessMouseMovement(xoffset, yoffset);
		}
		else {
			_data->window->GrabCursor(false);
			firstClick = true;
		}
	}


	if (target0.renderTarget.getWidth() != static_cast<uint32_t>(_data->window->GetWidth()) && target0.renderTarget.getHeight() != static_cast<uint32_t>(_data->window->GetHeight())) {
		// Wait the finish of the execution
		vkDeviceWaitIdle(_data->device);

		// Recreate the target
		target0.renderTarget.ResizeTarget(static_cast<uint32_t>(_data->window->GetWidth()), static_cast<uint32_t>(_data->window->GetHeight()), msaa_sample_count);
		// Change invalid pointer(previus target) with new one
		target0.drawDescriptor.SetImageElement(0, 0, { target0.renderTarget.getDescriptorInfo(linearSampler.get()) });

		vkDeviceWaitIdle(_data->device);
	}

	if (last_msaa_sample_count != msaa_sample_count) {
		last_msaa_sample_count = msaa_sample_count;

		vkDeviceWaitIdle(_data->device);

		// Recreate the target
		target0.renderTarget.ResizeTarget(static_cast<uint32_t>(_data->window->GetWidth()), static_cast<uint32_t>(_data->window->GetHeight()), msaa_sample_count);
		// Change invalid pointer(previus target) with new one
		target0.drawDescriptor.SetImageElement(0, 0, { target0.renderTarget.getDescriptorInfo(linearSampler.get()) });

		destroyPipelines();
		createPipelines();

		vkDeviceWaitIdle(_data->device);
	}

	glm::mat4 proj = glm::perspective(glm::radians(60.0f), viewport.width / viewport.height, 0.1f, 1000.0f);

	this->m_cameraData[_data->currentFrame].projection = proj;
	this->m_cameraData[_data->currentFrame].invProjection = glm::inverse(proj);
	this->m_cameraData[_data->currentFrame].view = camera.GetViewMatrix();
	this->m_cameraData[_data->currentFrame].invProjection = glm::inverse(camera.GetViewMatrix());
	this->m_cameraData[_data->currentFrame].viewPosition = glm::vec4(camera.Position, 0.0f);

	this->model_GPUDrawData.modelMatrix = glm::mat4(1.0f);
	this->model_GPUDrawData.modelMatrix = glm::translate(this->model_GPUDrawData.modelMatrix, this->model_transform.position);
	this->model_GPUDrawData.modelMatrix = glm::scale(this->model_GPUDrawData.modelMatrix, this->model_transform.scale);
	this->model_GPUDrawData.modelMatrix = glm::rotate(this->model_GPUDrawData.modelMatrix, this->model_transform.rotationAngle.x, glm::vec3(1, 0, 0));
	this->model_GPUDrawData.modelMatrix = glm::rotate(this->model_GPUDrawData.modelMatrix, this->model_transform.rotationAngle.y, glm::vec3(0, 1, 0));
	this->model_GPUDrawData.modelMatrix = glm::rotate(this->model_GPUDrawData.modelMatrix, this->model_transform.rotationAngle.z, glm::vec3(0, 0, 1));

	this->model_GPUDrawData.inverseTransposedModelMatrix = glm::transpose(glm::inverse(this->model_GPUDrawData.modelMatrix));

	this->model_GPUDrawColorData.lightSSBO_reference = this->m_lightSSBO[_data->currentFrame]->deviceAddress;
	this->model_GPUDrawColorData.samplerID = this->samplerIndex;
	this->model_GPUDrawColorData.material = fxCreateTextureMaterial(vec4(0.1f, 0.1f, 0.1f, 1.0f));

	void* data = this->internalMemory->MapMemory(this->m_cameraUBO[_data->currentFrame]);
	
	memcpy(data, &this->m_cameraData[_data->currentFrame], this->m_cameraUBO[_data->currentFrame]->size);

	this->internalMemory->UnMapMemory(this->m_cameraUBO[_data->currentFrame]);

	data = this->internalMemory->MapMemory(this->m_lightSSBO[_data->currentFrame]);
	
	memcpy(data, this->m_lightData.data(), this->m_lightSSBO[_data->currentFrame]->size);

	this->internalMemory->UnMapMemory(this->m_lightSSBO[_data->currentFrame]);
}

void ModelViewState::draw(VkCommandBuffer cmd)
{
	if (!isModelLoaded)
		return;

	// Render target pass

	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = static_cast<float>(target0.renderTarget.getWidth());
	viewport.height = static_cast<float>(target0.renderTarget.getHeight());

	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	float clearColor[] = {0.3f, 0.3f, 0.3f, 1.0f};
	target0.renderTarget.setRenderState(cmd, clearColor);

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	VkDescriptorSet descriptors[] = { this->descriptor.get(_data->currentFrame) };
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout.get(), 0, 1, descriptors, 0, nullptr);

	for (size_t i = 0; i < this->object_model->getNumChilds(); i++) {
		renderer::Model::Child child = this->object_model->getChild(i);
		
		child.mesh.bindIndexBuffer(cmd);
		child.mesh.bindVertexBuffer(cmd, 0);

		this->model_GPUDrawColorData.materialID = static_cast<uint32_t>(i);

		const uint32_t offset = static_cast<uint32_t>(xgr::aligned_size(sizeof(GPUDrawData), 16));
		vkCmdPushConstants(cmd, this->pipelineLayout.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawData), &this->model_GPUDrawData);
		vkCmdPushConstants(cmd, this->pipelineLayout.get(), VK_SHADER_STAGE_FRAGMENT_BIT, offset, sizeof(GPUDrawColorData), &this->model_GPUDrawColorData);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->generalPipeline.get());

		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(child.indices.size()), 1, 0, 0, 0);
	}

	target0.renderTarget.setShaderState(cmd);
}

void ModelViewState::drawMainPass(VkCommandBuffer cmd) 
{
	if (ImGui::BeginMainMenuBar()) 
	{
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Load file")) {
				std::string path;
				xgr::OpenNativeFileDialog(_data->window->GetHandle(), path);

				this->loadingThread->addJob([=]() { 
					this->loadModel(path); });
			}
			
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings")) 
		{
			if (ImGui::Checkbox("Fullscreen", &isFullscreen)) {
				_data->window->SetFullscreenMode(isFullscreen);
			}

			ImGui::Checkbox("Enable MSAA", &useMSAA);

			ImGui::BeginDisabled(!useMSAA);
			if (ImGui::BeginCombo("MSAA levels", string_VkSampleCountFlagBits(msaa_sample_count))) {
				const VkSampleCountFlagBits items[] = { VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_8_BIT };

				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					bool is_selected = (msaa_sample_count == items[n]);
					if (ImGui::Selectable(string_VkSampleCountFlagBits(items[n]), is_selected))
						msaa_sample_count = items[n];

					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
			ImGui::EndDisabled();

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	auto loadingCondition = this->loadingThread->getQueueSize() != 0;

	if (loadingCondition)
		ImGui::OpenPopup("_LoadingPopup");
	
	if (ImGui::BeginPopupModal("_LoadingPopup", &loadingCondition, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
		ImGui::SetItemDefaultFocus();
		ImGui::Text("Loading %c", "|/-\\"[(int)(ImGui::GetTime() / 0.5f) & 3]);
		ImGui::EndPopup();
	}

	ImGui::Begin("View Settings");

	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning! \nLoading complex models may take a \nlot of time to process...");

	if (ImGui::CollapsingHeader("3D Camera")) {
		ImGui::Checkbox("Use Free Camera", &useFreeCamera);
	}

	if (ImGui::CollapsingHeader("Shading")) {
		ImGui::Checkbox("Show Diffuse", (bool*)&this->model_GPUDrawColorData.showDiffuse);
		ImGui::Checkbox("Show Normal", (bool*)&this->model_GPUDrawColorData.showNormal);
		ImGui::Checkbox("Show Specular", (bool*)&this->model_GPUDrawColorData.showSpecular);
	}

	if (ImGui::CollapsingHeader("Transformation")) {
		float min_value = -400.0f;
		float max_value = 400.0f;

		ImGui::Text("Model Position");
		ImGui::SliderFloat("Pos X", &this->model_transform.position.x, min_value, max_value);
		ImGui::SliderFloat("Pos Y", &this->model_transform.position.y, min_value, max_value);
		ImGui::SliderFloat("Pos Z", &this->model_transform.position.z, min_value, max_value);
		ImGui::NewLine();

		ImGui::Text("Model Scale");
		ImGui::SliderFloat("Scl X", &this->model_transform.scale.x, min_value, max_value);
		ImGui::SliderFloat("Scl Y", &this->model_transform.scale.y, min_value, max_value);
		ImGui::SliderFloat("Scl Z", &this->model_transform.scale.z, min_value, max_value);
		ImGui::NewLine();

		ImGui::Text("Model Rotation(in degrees)");
		ImGui::SliderAngle("Rot X", &this->model_transform.rotationAngle.x);
		ImGui::SliderAngle("Rot Y", &this->model_transform.rotationAngle.y);
		ImGui::SliderAngle("Rot Z", &this->model_transform.rotationAngle.z);
	}

	ImGui::End();

	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = static_cast<float>(_data->window->GetWidth());
	viewport.height = static_cast<float>(_data->window->GetHeight());

	scissor.offset = { 0, 0 };
	scissor.extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };

	vkCmdSetViewport(cmd, 0, 1, &viewport);
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	VkDescriptorSet bindingSet[] = { target0.drawDescriptor.get(_data->currentFrame) };
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, target0.drawPipelineLayout.get(), 0, 1, bindingSet, 0, nullptr);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, target0.drawPipeline.get());
	vkCmdDraw(cmd, 3, 1, 0, 0);
}

void ModelViewState::destroy() {
	this->loadingThread.reset();

	linearSampler.destroy();
	pointSampler.destroy();

	destroyPipelines();
	descriptor.destroy();
	descriptorPool.destroy();
	pipelineLayout.destroy();

	pipelineCache.destroy();

	target0.descriptorPool.destroy();
	target0.drawDescriptor.destroy();
	target0.drawPipelineLayout.destroy();
	target0.drawPipeline.destroy();
	target0.renderTarget.destroy();

	object_model->destroy();

	internalMemory->destroy();
	sceneMemory->destroy();
	imageMemory->destroy();

	delete internalMemory;
	delete sceneMemory;
	delete imageMemory;
	delete object_model;

	this->options_file.SaveFileData();
}