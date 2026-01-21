#ifndef LINEREADER_HPP
#define LINEREADER_HPP

class LineReader {
public:
    explicit LineReader(std::istream& stream) : _istream(stream) {}

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
            _istream.read(buffer, sizeof(buffer));
            std::streamsize n = _istream.gcount();

            if (n == 0) {
                if (!_stash.empty()) {
                    out = _stash;
                    _stash.clear();
                    return true;
                }
                return false;
            }

            if (_istream.bad()) {
                std::cout << "error: read failure" << std::endl;
                return false;
            }

            _stash.append(buffer, static_cast<std::size_t>(n));
        }
    }

private:
    std::istream& _istream;
    std::string _stash;
};


#endif