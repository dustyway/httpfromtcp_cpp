#include <cstring>
#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("messages.txt");
    if (!file) {
        std::cout << "Error opening file" << std::endl;
        return 1;
    }

    char buffer[8];
    std::string currentLine;


    while (true) {
        file.read(buffer, sizeof(buffer));
        std::streamsize n = file.gcount();

        if (n > 0) {
            const char* found = static_cast<const char*>(memchr(buffer, '\n', n));
            if (found) {
                currentLine.append(buffer,  found-buffer);
                std::cout << "read: " << currentLine << std::endl;
                currentLine = std::string(found+1,  n - (found-buffer) - 1);
            } else {
                currentLine.append(buffer,  n);
            }
        }

        if (file.eof()) {
            break;
        }

        if (file.fail()) {
            std::cout << "Error reading file" << std::endl;
            return 1;
        }
    }
    if (!currentLine.empty())
        std::cout << "read: " << currentLine << std::endl;
    file.close();
    return 0;
}
