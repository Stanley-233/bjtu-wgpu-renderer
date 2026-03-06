//
// Created by Stanley on 2026/3/6.
//

#include <GLFW/glfw3.h>
#include "../ext/glfw3webgpu/glfw3webgpu.h"

#include <iostream>

#include "application.h"

bool Application::Initialize() {
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(640, 480, "WebGPU Renderer", nullptr, nullptr);

	wgpu::raii::Instance instance = wgpu::Instance(wgpuCreateInstance(nullptr));

	std::cout << "Requesting adapter..." << std::endl;
	surface = wgpu::Surface(glfwGetWGPUSurface(*instance, window));
	wgpu::RequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = *surface;
	wgpu::raii::Adapter adapter = instance->requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
	wgpu::DeviceDescriptor deviceDesc = {};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	device = adapter->requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	uncapturedErrorCallbackHandle = device->setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	});

	queue = device->getQueue();

	// Configure the surface
	wgpu::SurfaceConfiguration config = {};

	// Configuration of the textures created for the underlying swap chain
	config.width = 640;
	config.height = 480;
	config.usage = wgpu::TextureUsage::RenderAttachment;
	wgpu::TextureFormat surfaceFormat = surface->getPreferredFormat(*adapter);
	config.format = surfaceFormat;

	// And we do not need any particular view format:
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = *device;
	config.presentMode = wgpu::PresentMode::Fifo;
	config.alphaMode = wgpu::CompositeAlphaMode::Auto;

	surface->configure(config);

	// Release the adapter only after it has been fully utilized
	// adapter.release();

	return true;
}

void Application::Terminate() {
	surface->unconfigure();
	/*queue.release();
	surface.release();
	device.release();*/
	glfwDestroyWindow(window);
	glfwTerminate();
}

bool Application::IsRunning() const {
	return !glfwWindowShouldClose(window);
}

void Application::MainLoop() {
	glfwPollEvents();

	// Get the next target texture view
	wgpu::raii::TextureView targetView = GetNextSurfaceTextureView();
	if (!targetView) return;

	// Create a command encoder for the draw call
	wgpu::CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = "My command encoder";
	wgpu::raii::CommandEncoder encoder = wgpu::CommandEncoder(wgpuDeviceCreateCommandEncoder(*device, &encoderDesc));

	// Create the render pass that clears the screen with our color
	wgpu::RenderPassDescriptor renderPassDesc = {};

	// The attachment part of the render pass descriptor describes the target texture of the pass
	wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = *targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
#ifndef WEBGPU_BACKEND_WGPU
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	wgpu::raii::RenderPassEncoder renderPass = encoder->beginRenderPass(renderPassDesc);
	renderPass->end();
	// renderPass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.label = "Command buffer";
	wgpu::raii::CommandBuffer command_buffer = encoder->finish(cmdBufferDescriptor);
	// encoder.release();

	if (enableMainLoopDebug) {
		std::cout << "Submitting command..." << std::endl;
	}
	queue->submit(1, &*command_buffer);
	// command.release();
	if (enableMainLoopDebug) {
		std::cout << "Command submitted." << std::endl;
	}

	// At the end of the frame
	// targetView.release();
#ifndef __EMSCRIPTEN__
	surface->present();
#endif

#ifdef WEBGPU_BACKEND_DAWN
	device->tick();
#elif WEBGPU_BACKEND_WGPU
	device->poll(false);
#endif
}

wgpu::raii::TextureView Application::GetNextSurfaceTextureView() {
	wgpu::SurfaceTexture surfaceTexture;
	surface->getCurrentTexture(&surfaceTexture);
	if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
		return {};
	}
	wgpu::Texture texture = surfaceTexture.texture;

	// Create a view for this surface texture
	wgpu::TextureViewDescriptor viewDescriptor;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = wgpu::TextureAspect::All;
	wgpu::raii::TextureView targetView = texture.createView(viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
	// We no longer need the texture, only its view
	// (NB: with wgpu-native, surface textures must not be manually released)
	wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU

	return targetView;
}