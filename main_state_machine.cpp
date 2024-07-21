// state machine version
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

const std::string open{"{"};
const std::string close{"{"};

class Block {
   private:
    int m_n{0};
    std::vector<std::string> m_stack;
    int m_counter{0};
    class State {
       public:
        virtual ~State() = default;
        virtual void handle() = 0;
    };
    class StateOff: public State {
        void handle() override {

        };
    };
    class StateOn: public State {
        void handle() override {

        };
    };
    StateOff m_state_off{};
    StateOn m_state_on{};
    State &m_state = m_state_off;
   public:
    Block(int n) : m_n(n){};
    void reader() {
        std::string command;
        while (std::getline(std::cin, command)) {
            if (command.empty()) {
                break;
            }
            if (command == open) {
                m_state = m_state_on;
                // TODO: state on
            } else if (command == close) {
                // TODO: state off
                m_state = m_state_off;
            } else {
                // in block
                m_stack.emplace_back(std::move(command));
                m_counter++;
            }
        }
    };
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
};

// class StateOn : public State {
//    public:
//     void state(Block& block) override {
//         // Implement state On behavior
//     }
// };

// class StateOff : public State {
//    public:
//     void state(Block& block) override {
//         // Implement state Off behavior
//     }
// };

// class BlockOff : public Block {
//    public:
//     BlockOff(int n) : Block(n){};
//     void reader(State& state) override {
//         std::string command;
//         while (std::getline(std::cin, command)) {
//             if (command.empty()) {
//                 break;
//             }
//             if (command == m_open) {
//                 print();
//                 clear();
//                 // TODO: state On
//             } else {
//                 // in block
//                 m_stack.emplace_back(std::move(command));
//                 m_counter++;
//                 if (m_counter == m_n) {
//                     print();
//                     clear();
//                 }
//             }
//         }
//         print();
//         return;
//     };
// };
// class BlockOn : public Block {
//    private:
//     int m_nesting{0};

//    public:
//     BlockOn(int n) : Block(n){};
//     void reader(State& state) override {
//         std::string command;
//         while (std::getline(std::cin, command)) {
//             if (command.empty()) {
//                 break;
//             }
//             if (command == m_open) {
//                 // TODO: state Inner
//                 m_nesting++;
//                 m_counter = 0;
//             } else if (command == m_close) {
//                 // TODO: state off
//                 m_nesting--;
//                 if (m_nesting == 0) {
//                     print();
//                     clear();
//                 }
//             } else {
//                 // in block
//                 m_stack.emplace_back(std::move(command));
//                 m_counter++;
//                 if (m_counter > m_n) {
//                     clear();
//                 }
//             }
//         }
//         return;
//     };
// };

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    int N = std::stoi(argv[1]);

    return 0;
}
