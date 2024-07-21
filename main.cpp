#include <algorithm>
#include <deque>
#include <iostream>
#include <iterator>
#include <string>

class Status {
   public:
    enum class BLOCK : int { OFF = 0, ON = 1, INNER = 2 };

   private:
    using my_type = std::deque<std::string>;
    my_type m_stack;
    int m_n{3};
    int m_counter{0};
    BLOCK m_block_status{BLOCK::OFF};
    my_type::iterator begin() { return m_stack.begin(); };
    my_type::iterator end() { return m_stack.end(); };

    void print() {
        if (m_stack.size() > 0) {
            std::copy(m_stack.begin(), m_stack.end(), std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
        }        
    };

    void clear() {
        m_stack.clear();
        m_counter = 0;
    };

   public:
    Status(int N, BLOCK block_status) : m_n(N), m_block_status(block_status){};
    ~Status() {
        std::cout << "D:" << static_cast<int>(m_block_status) << "\n";        
        if (m_block_status != BLOCK::INNER) {
            print();
        }
        clear();
    };

    void reader() {
        std::string command;
        while (std::getline(std::cin, command)) {
            if (command.empty()) {
                break;
            }
            if (command == "{") {
                if (m_block_status == BLOCK::OFF) {
                    // close block
                    print();
                    clear();
                }
                // new block
                Status inner_reader(m_n, static_cast<BLOCK>(static_cast<int>(m_block_status) + 1));
                inner_reader.reader();
                if (m_block_status > BLOCK::OFF) {
                    for (auto&& v : inner_reader) {
                        m_stack.emplace_back(std::move(v));
                    };
                } else {
                    inner_reader.print();
                }
                // call dtor
            } else if (command == "}") {
                // close block
                if (m_block_status != BLOCK::OFF) {
                    return;
                }
            } else {
                // in block
                m_stack.emplace_back(std::move(command));
                m_counter++;
                if (m_counter == m_n) {
                    if (m_block_status > BLOCK::OFF) {
                        clear();
                    } else {
                        print();
                        clear();
                    }
                };
            }
        }
        return;
    };
};

int main(int argc, char* argv[]) {
    // if (argc != 2) {
    //     std::cerr << "Usage: " << argv[0] << " N" << std::endl;
    //     return 1;
    // }
    // int N = 3;
    // try {
    //     N = std::stoi(argv[1]);
    // } catch (const std::exception& ex) {
    //     std::cerr << "Usage: " << argv[0] << " N" << std::endl;
    //     return 1;
    // }
    int N = 3;
    Status read(N, Status::BLOCK::OFF);
    read.reader();
    return 0;
}
