#include "File.h"

File::File(const std::string& file_name) : file_(file_name) {
    LOG_TRACE("File constructor");
    if (!file_.is_open()) {
        LOG_ERROR("Failed to open file: {}", file_name);
        return;
    }
}

File::~File() {
    LOG_TRACE("File destructor");
    file_.close();
}

std::string File::getLine() {
    std::string line;
    std::getline(file_, line);
    return line;
}

std::vector<std::string> File::getVectorOfLines() {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file_, line)) {
        lines.push_back(line);
    }
    return lines;
}

std::ifstream& File::getContent() {
    return std::ref(file_);
}
