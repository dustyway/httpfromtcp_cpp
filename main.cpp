#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("messages.txt");
    if (!file) {
        std::cout << "Error opening file" << std::endl;
        return 1;
    }

      char buffer[8];
      while (true) {
          file.read(buffer, sizeof(buffer));
          std::streamsize n = file.gcount();

          // If we read some bytes, print them
          if (n > 0) {
              std::cout << "read: ";
              std::cout.write(buffer, n);
              std::cout << std::endl;
          }

          // If we hit EOF, exit the program
          if (file.eof()) {
              break;
          }

          // If there's any other error, report it
          if (file.fail()) {
              std::cout << "Error reading file" << std::endl;
              return 1;
          }
      }

      file.close();
      return 0;
  }