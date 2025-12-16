#ifndef FILE_H
#define FILE_H

#include <fstream>
#include <string>
#include "Logger/Logger.h"

class File {
public:
    explicit File(const std::string& file_name);
    ~File();
    std::string getLine();
    std::vector<std::string> getVectorOfLines();
    std::ifstream& getContent();
private:
    std::ifstream file_;
};

#endif //FILE_H
