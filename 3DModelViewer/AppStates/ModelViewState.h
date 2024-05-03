#pragma once

#include <XGREngine/AppState.h>
#include <XGREngine/ThreadPool.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <XGREngine/Renderer/RenderTarget.h>
#include <XGREngine/Renderer/Model.h>
#include <XGREngine/Renderer/Camera.h>
#include <XGREngine/Memory/Descriptor.h>
#include <XGREngine/Memory/BufferMemory.h>
#include <XGREngine/Memory/ImageMemory.h>

#include <XGR/pipeline/XPipeline.h>
#include <XGR/pipeline/XPipelineLayout.h>
#include <XGR/pipeline/XPipelineCache.h>
#include <XGR/pipeline/XSampler.h>

#include "../Shaders/GlobalDef.h"
#include <XGREngine/Utils/ConfigFile.h>

namespace states {
	class ModelViewState : public XGREngine::AppState
	{
	private:
		struct RenderTarget
		{
			xgr::XPipelineLayout drawPipelineLayout;
			xgr::XPipeline drawPipeline;
			xgr::XDescriptorPool descriptorPool;
			XGREngine::memory::Descriptor drawDescriptor;
			XGREngine::renderer::RenderTarget renderTarget;
		};

		struct GPUDrawData {
			glm::mat4 modelMatrix;
			glm::mat4 inverseTransposedModelMatrix;
		} model_GPUDrawData{};

		struct GPUDrawColorData {
			Material material;
			uint64_t lightSSBO_reference;
			uint32_t samplerID;
			uint32_t materialID;

			uint32_t showDiffuse;
			uint32_t showNormal;
			uint32_t showSpecular;
		} model_GPUDrawColorData{};

		struct Transform {
			glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
			glm::vec3 rotationAngle = { 0.0f, 0.0f, 0.0f };
			glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		} model_transform;

		XGREngine::ConfigFile options_file;

	private:
		VkSampleCountFlagBits msaa_sample_count;
		VkSampleCountFlagBits last_msaa_sample_count;

		VkViewport viewport;
		VkRect2D scissor;

		bool isFullscreen = false;
		bool useFreeCamera = true;
		bool useMSAA = false;
		std::atomic_bool isModelLoaded = false;
		uint32_t samplerIndex = 0;

		std::unique_ptr<XGREngine::Thread> loadingThread;

		XGREngine::renderer::Camera camera;

		XGREngine::memory::BufferMemory* internalMemory;
		XGREngine::memory::BufferMemory* sceneMemory;
		XGREngine::memory::ImageMemory* imageMemory;
		RenderTarget target0;

		xgr::XSampler linearSampler;
		xgr::XSampler pointSampler;

		xgr::XPipelineLayout pipelineLayout;
		XGREngine::memory::Descriptor descriptor;
		xgr::XDescriptorPool descriptorPool;

		xgr::XPipelineCache pipelineCache;
		xgr::XPipeline generalPipeline;

		// Camera UBO
		std::vector<XGREngine::memory::AllocatedBuffer*> m_cameraUBO;
		std::vector<CameraMatrix> m_cameraData;

		// Light SSBO
		std::vector<XGREngine::memory::AllocatedBuffer*> m_lightSSBO;
		std::vector<Light> m_lightData;

		XGREngine::renderer::Model* object_model;

	private:
		void createRenderTargetPipeline();

		void createPipelines();
		void destroyPipelines();

		void loadModel(std::string modelName);

	public:
		virtual void init() override;

		virtual void update(float dt) override;

		virtual void draw(VkCommandBuffer cmd) override;

		virtual void drawMainPass(VkCommandBuffer cmd) override;

		virtual void destroy() override;
	};
}