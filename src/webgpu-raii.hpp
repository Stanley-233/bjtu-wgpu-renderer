/**
 * This is a RAII wrapper for WebGPU native API.
 *
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2025 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <webgpu/webgpu.hpp>

// Emscripten still uses the old 'reference' method, which has been renamed 'addRef'
#ifdef __EMSCRIPTEN__
#  define addRef reference
#endif


namespace wgpu::raii {
    /**
     * RAII wrapper around a raw WebGPU type.
     * Use pointer-like dereferencing to access methods from the wrapped type.
     */
    template<typename Raw>
    class Wrapper {
    public:
        Wrapper()
            : m_raw(nullptr) {
        }

        Wrapper(Raw &&raw)
            : m_raw(raw) {
        }

        // We define a destructor...
        ~Wrapper() {
            Destruct();
        }

        // Copy semantics
        Wrapper &operator=(const Wrapper &other) {
            if (this == &other) return *this;
            Destruct();
            assert(m_raw == nullptr);
            m_raw = other.m_raw;
            if (m_raw != nullptr) {
                m_raw.addRef();
            }
            return *this;
        }

        Wrapper(const Wrapper &other)
            : m_raw(other.m_raw) {
            if (m_raw != nullptr) {
                m_raw.addRef();
            }
        }

        // Move semantics
        Wrapper &operator=(Wrapper &&other) noexcept {
            if (this == &other) return *this;
            Destruct();
            assert(m_raw == nullptr);
            m_raw = other.m_raw;
            other.m_raw = nullptr;
            return *this;
        }

        Wrapper(Wrapper &&other)
            noexcept : m_raw(other.m_raw) {
            other.m_raw = nullptr;
        }

        Raw* Ptr() {
            return &m_raw;
        }

        friend std::ostream& operator<<(std::ostream& os, const Wrapper& wrapper) {
            if (wrapper.m_raw) {
                os << wrapper.m_raw;
            } else {
                os << "nullptr";
            }
            return os;
        }

        explicit operator bool() const { return m_raw != nullptr; }
        const Raw &operator*() const { return m_raw; }
        Raw &operator*() { return m_raw; }
        const Raw *operator->() const { return &m_raw; }
        Raw *operator->() { return &m_raw; }

    private:
        void Destruct() {
            if (!m_raw) return;
            m_raw.release();
            m_raw = nullptr;
        }

    private:
        // Raw resources that is wrapped by the RAII class
        Raw m_raw;
    };

    using Adapter = Wrapper<Adapter>;
    using BindGroup = Wrapper<BindGroup>;
    using BindGroupLayout = Wrapper<BindGroupLayout>;
    using Buffer = Wrapper<Buffer>;
    using CommandBuffer = Wrapper<CommandBuffer>;
    using CommandEncoder = Wrapper<CommandEncoder>;
    using ComputePassEncoder = Wrapper<ComputePassEncoder>;
    using ComputePipeline = Wrapper<ComputePipeline>;
    using Device = Wrapper<Device>;
    using Instance = Wrapper<Instance>;
    using PipelineLayout = Wrapper<PipelineLayout>;
    using QuerySet = Wrapper<QuerySet>;
    using Queue = Wrapper<Queue>;
    using RenderBundle = Wrapper<RenderBundle>;
    using RenderBundleEncoder = Wrapper<RenderBundleEncoder>;
    using RenderPassEncoder = Wrapper<RenderPassEncoder>;
    using RenderPipeline = Wrapper<RenderPipeline>;
    using Sampler = Wrapper<Sampler>;
    using ShaderModule = Wrapper<ShaderModule>;
    using Surface = Wrapper<Surface>;
    using Texture = Wrapper<Texture>;
    using TextureView = Wrapper<TextureView>;
} // namespace wgpu::raii
