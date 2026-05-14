#include "ConfigPaths.h"

#include <array>
#include <string>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace {

[[maybe_unused]] std::filesystem::path ExecutableDir() {
#if defined(_WIN32)
    std::array<wchar_t, 4096> buffer{};
    const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0 || size == buffer.size()) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer.data(), buffer.data() + size).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    (void)_NSGetExecutablePath(nullptr, &size);
    if (size == 0) {
        return std::filesystem::current_path();
    }

    std::string path(size, '\0');
    if (_NSGetExecutablePath(path.data(), &size) != 0) {
        return std::filesystem::current_path();
    }

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(std::filesystem::path(path.c_str()), ec);
    if (ec) {
        canonical = std::filesystem::path(path.c_str());
    }
    return canonical.parent_path();
#else
    std::array<char, 4096> buffer{};
    const ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (size <= 0) {
        return std::filesystem::current_path();
    }
    buffer[static_cast<size_t>(size)] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();
#endif
}

} // namespace

std::filesystem::path ConfigPaths::ConfigRoot() {
#ifdef __EMSCRIPTEN__
    return "/config";
#elif defined(CONFIG_DIR)
    return std::filesystem::path(CONFIG_DIR);
#else
    return ExecutableDir() / "config";
#endif
}

std::filesystem::path ConfigPaths::Resolve(const std::filesystem::path& relativePath) {
    return ConfigRoot() / relativePath;
}
