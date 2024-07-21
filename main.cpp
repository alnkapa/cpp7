#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

class Status {
   private:
    std::vector<std::string> m_stack;
    int m_n{3};
    int m_counter{0};
    bool m_inner_block{false};
    enum class Token { NO_SET, OPEN_BRACKET, CLOSE_BRACKET };

   public:
    Status(int N, bool inner_block) : m_n(N), m_inner_block(inner_block) { m_stack.reserve(N); };
    ~Status(){
        if (!m_inner_block) {
            print();
        }
    };
    void print() {
        if (m_counter > 0) {
            std::copy_n(m_stack.begin(), m_counter, std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
            m_stack.clear();
            m_counter = 0;
        }
    }
    Token reader() {
        std::string command;
        while (std::getline(std::cin, command)) {
            if (command.empty()) {
                break;
            }
            if (command == "{") {
                // TODO: close block

                // new block
                Status read(m_n, true);
                auto token = read.reader();
            } else if (command == "}") {
                // close block
                return Token::CLOSE_BRACKET;
            } else {
                // in block
                m_stack.emplace_back(std::move(command));
                m_counter++;
                if (m_counter == m_n) {
                    print();
                };
            }
        }
        return Token::NO_SET;
    };
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    int N = 3;
    try {
        N = std::stoi(argv[1]);
    } catch (const std::exception& ex) {
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    Status read(N, false);
    read.reader();
    return 0;
}
