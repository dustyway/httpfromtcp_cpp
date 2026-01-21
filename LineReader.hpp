#ifndef LINEREADER_HPP
#define LINEREADER_HPP

class LineReader {
public:
    explicit LineReader(int fd) : _fd(fd) {}

    bool getLine(std::string& out) {
        while (true) {
            if (!_stash.empty()) {
                std::string::size_type pos = _stash.find('\n');
                if (pos != std::string::npos) {
                    out = _stash.substr(0, pos);
                    _stash.erase(0, pos + 1);
                    return true;
                }
            }

            char buffer[8];
            ssize_t n = read(_fd, buffer, sizeof(buffer));

            if (n < 0)
            {
                std::cout << "error: " << strerror(errno) << std::endl;
                return false;
            }

            if (n == 0) {
                if (!_stash.empty()) {
                    out = _stash;
                    _stash.clear();
                    return true;
                }
                return false;
            }

            _stash.append(buffer, static_cast<std::size_t>(n));
        }
    }

private:
    int _fd;
    std::string _stash;
};


#endif