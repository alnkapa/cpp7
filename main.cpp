#include <iostream>
#include <iterator>
#include <string>
#include <vector>

class Status {
   public:
    enum class BLOCK : int { OFF = 0, ON = 1, INNER = 2 };

   private:
    using my_type = std::vector<std::string>;
    my_type m_stack;
    int m_n{3};
    int m_counter{0};
    BLOCK m_block_hierarchy{BLOCK::OFF};
    my_type::iterator begin() { return m_stack.begin(); };
    my_type::iterator end() { return m_stack.end(); };
    bool m_block_end{false};
    void print() {
        if (m_stack.size() > 0) {
            std::cout << "bulk: ";
            auto it = m_stack.begin();
            while (it != m_stack.end()) {
                std::cout << *it;
                if (++it != m_stack.end()) {
                    std::cout << ", ";
                }
            }            
            std::cout << std::endl;
        }
    };

    void clear() {
        m_stack.clear();
        m_counter = 0;
    };

   public:
    Status(int N, BLOCK block_status) : m_n(N), m_block_hierarchy(block_status){};
    ~Status() {
        if (m_block_hierarchy == BLOCK::OFF) {
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
                if (m_block_hierarchy == BLOCK::OFF) {
                    print();
                    clear();
                }
                // new block
                Status inner_reader(m_n, static_cast<BLOCK>(static_cast<int>(m_block_hierarchy) + 1));
                inner_reader.reader();
                if (m_block_hierarchy > BLOCK::OFF) {
                    for (auto&& v : inner_reader) {
                        m_stack.emplace_back(std::move(v));
                    };
                } else if (inner_reader.m_block_end) {
                    inner_reader.print();
                }
                // call dtor
            } else if (command == "}") {
                m_block_end = true;
                // close block
                if (m_block_hierarchy != BLOCK::OFF) {
                    return;
                }
            } else {
                // in block
                m_stack.emplace_back(std::move(command));
                m_counter++;
                if (m_block_hierarchy == BLOCK::OFF && m_counter == m_n) {
                    print();
                    clear();
                } else if (m_block_hierarchy > BLOCK::OFF && m_counter > m_n) {
                    clear();
                }
            }
        }
        return;
    };
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    int N = std::stoi(argv[1]);
    Status read(N, Status::BLOCK::OFF);
    read.reader();
    return 0;
}
