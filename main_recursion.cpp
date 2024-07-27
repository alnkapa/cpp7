// recursion version
#include <chrono>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

class TeeStream : public std::ostream {
   private:
    std::vector<std::ostream*> m_out;

   public:
    TeeStream(std::initializer_list<std::ostream*> out) : std::ostream(nullptr), m_out(out){};
    void add(std::ostream* out) { m_out.emplace_back(out); };
    template <typename T>
    TeeStream& operator<<(T in) {
        for (std::ostream* out : m_out) {
            (*out) << in;
        }
        return *this;
    };
    inline TeeStream& operator<<(std::ostream& (*__pf)(std::ostream&)) {
        for (std::ostream* out : m_out) {
            (*out) << __pf;
        }
        return *this;
    }
    // TODO: make close for file in destructor
};

class Status {
   public:
    enum class BLOCK : int { OFF = 0, ON = 1, INNER = 2 };

   private:
    std::vector<std::string> m_stack;
    long long m_time_stamp{0};
    int m_n{3};
    int m_counter{0};
    BLOCK m_block_hierarchy{BLOCK::OFF};
    bool m_block_end{false};
    void print() {
        if (m_stack.size() > 0) {
            TeeStream multiStream{&std::cout};        
            std::ofstream file("bulk" + std::to_string(m_time_stamp) + ".log");
            if (file.is_open()) {
                multiStream.add(&file);
            }
            multiStream << "bulk: ";
            auto it = m_stack.begin();
            while (it != m_stack.end()) {
                multiStream << *it;
                if (++it != m_stack.end()) {
                    multiStream << ", ";
                }
            }
            multiStream << std::endl;
            if (file.is_open()) {
                file.close();
            }
        }
    };

    void clear() {
        m_stack.clear();
        m_counter = 0;
        m_time_stamp = 0;
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
                auto block_hierarchy = m_block_hierarchy;
                if (m_block_hierarchy < BLOCK::INNER) {
                    block_hierarchy = static_cast<BLOCK>(static_cast<int>(m_block_hierarchy) + 1);
                }
                Status inner_reader(m_n, block_hierarchy);
                inner_reader.reader();
                if (m_block_hierarchy > BLOCK::OFF) {
                    for (auto&& v : inner_reader.m_stack) {
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
                if (m_time_stamp == 0) {
                    m_time_stamp = std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count();
                }
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
