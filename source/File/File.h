#ifndef FILE_H
#define FILE_H

#include <fstream>
#include <string>
#include "Gstreamer/PipelineElement.h"
#include "Logger/Logger.h"

class File {
public:
    explicit File(const std::string& file_name);
    ~File();
    std::string get_line();
    std::vector<std::string> get_vector_of_lines();
    std::ifstream& get_content();
private:
    std::ifstream file_;
};

#endif //FILE_H
