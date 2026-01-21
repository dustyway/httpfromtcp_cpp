#include <iostream>
#include <fstream>
#include "LineReader.hpp"

int main() {
    std::ifstream file("messages.txt");
    if (!file.is_open()) {
        std::cout << "Error opening file:" << std::endl;
        return 1;
    }

    LineReader reader(file);
    std::string line;
    while (reader.getLine(line)) {
        std::cout << "read: " << line << std::endl;
    }

    file.close();
    return 0;
}
