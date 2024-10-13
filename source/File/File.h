#ifndef FILE_H
#define FILE_H

#include <fstream>
#include <string>
#include "Logger/Logger.h"

class File {
public:
    explicit File(const std::string& file_name);
    ~File();
    std::string get_line();
    std::vector<std::string> get_vector_of_lines();
private:
    std::ifstream file_;
};

#endif //FILE_H
