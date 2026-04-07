#include "LegacyTxtGeometryLoader.h"

#include <fstream>
#include <sstream>
#include <string>

bool LegacyTxtGeometryLoader::Load(
    const std::filesystem::path& path,
    std::vector<float>&          pointData,
    std::vector<uint16_t>&       indexData
) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class ESection {
        None,
        Points,
        Indices,
    };
    ESection currentSection = ESection::None;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') {
            continue;
        }
        if (line == "[points]") {
            currentSection = ESection::Points;
            continue;
        }
        if (line == "[indices]") {
            currentSection = ESection::Indices;
            continue;
        }

        if (currentSection == ESection::Points) {
            std::istringstream iss(line);
            for (int i = 0; i < 5; ++i) {
                float value = 0.0f;
                iss >> value;
                pointData.push_back(value);
            }
            continue;
        }

        if (currentSection == ESection::Indices) {
            std::istringstream iss(line);
            for (int i = 0; i < 3; ++i) {
                uint16_t index = 0;
                iss >> index;
                indexData.push_back(index);
            }
        }
    }

    return true;
}
