#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <iostream>
#include <vector>

#include "utils.h"

int main(int, char **) {
	// Descriptor serves as a configuration for the instance.
	// It is optional, but may be used to specify additional features or limits.
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

	// Emscripten requires nullptr as default descriptor
#ifdef __EMSCRIPTEN__
	WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
	WGPUInstance instance = wgpuCreateInstance(&desc);
#endif

	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}
	std::cout << "WGPU instance: " << instance << std::endl;

	std::cout << "Requesting adapter..." << std::endl;
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;
	inspectAdapter(adapter);
	wgpuInstanceRelease(instance);

	std::cout << "Requesting device..." << std::endl;
	WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device"; // anything works here, that's your call
	deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
	deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	// A function that is invoked whenever the device stops being available.
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);
	std::cout << "Got device: " << device << std::endl;
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);
	wgpuAdapterRelease(adapter);
	inspectDevice(device);

	wgpuDeviceRelease(device);

	return 0;
}
