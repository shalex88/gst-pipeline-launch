#pragma once

#include <fstream>
#include <string>

#include "common/logger/Logger.h"

class File {
public:
    explicit File(std::string_view file_name);
    ~File();
    std::string getLine();
    std::vector<std::string> getVectorOfLines();
    std::ifstream& getContent();
private:
    std::ifstream file_;
};