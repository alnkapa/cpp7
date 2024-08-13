// sub-pub version
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

template <typename T>
class Subscriber {
   public:
    using value_type = T;
    // TODO:: callback(value_type&&)
    virtual void callback(const value_type& message) = 0;
    virtual ~Subscriber() = default;
};

template <typename T>
class Publisher {
    using subscriber_type = Subscriber<T>;
    using value_type = Subscriber<T>::value_type;

   public:
    virtual ~Publisher() = default;
    void subscribe(const std::weak_ptr<subscriber_type> in) { subscribers_array.push_back(in); }
    void unsubscribe(const std::weak_ptr<subscriber_type>& in) {
        subscribers_array.erase(std::remove_if(subscribers_array.begin(), subscribers_array.end(),
                                               [&in](const std::weak_ptr<subscriber_type>& v) {
                                                   return v.expired() || in.lock() == v.lock();
                                               }),
                                subscribers_array.end());
    }
    // TODO:: notify(value_type&&)
    void notify(const value_type& in) {
        for (auto it = subscribers_array.begin(); it != subscribers_array.end();) {
            if (auto l = it->lock()) {
                l->callback(in);
                ++it;
            } else {
                it = subscribers_array.erase(it);
            }
        }
    }

   private:
    std::vector<std::weak_ptr<subscriber_type>> subscribers_array;
};

// ждем ввода команды из std::cin
using unix_time_stamp_t = long long;
enum class Status { NONE, BLOCK_ON, BLOCK_OFF };
enum class Level { NONE, FIRST, OTHER };
using command_t = std::tuple<Status, std::string, unix_time_stamp_t>;
using print_t = std::pair<unix_time_stamp_t, std::vector<std::string>>;
class CinPub : public Publisher<command_t> {
   public:
    unix_time_stamp_t time() {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
    void run() {
        std::string command;
        while (std::getline(std::cin, command)) {
            if (command.empty()) {
                break;
            }
            if (command == "{") {  // начало блока
                notify({Status::BLOCK_ON, "", 0});
            } else if (command == "}") {  // конец блока
                notify({Status::BLOCK_OFF, "", 0});
            } else {  // команда
                notify({Status::NONE, command, time()});
            }
        }
    };
};

template <Level L>
class Active {
   private:
    int m_curent_level{0};

   public:
    Active() {};
    bool On(Status in) {
        switch (in) {
            case Status::BLOCK_ON:
                ++m_curent_level;
                break;
            case Status::BLOCK_OFF:
                --m_curent_level;
                break;
            default:
                break;
        }
        return L == m_curent_level;
    };
    bool Up() { return m_curent_level - L == 1; }
    bool Down() { return m_curent_level - L == -1; }
};

class Cmd : public Subscriber<command_t>, public Publisher<print_t> {};

// без блока
class OFFSub : public Cmd {
   private:
    std::vector<std::string> m_stack;
    unix_time_stamp_t m_time_stamp{0};
    size_t m_n{3};
    size_t m_counter{0};
    Active<Level::NONE> active{};
    void pub_and_clear() {
        if (!m_stack.empty()) {
            notify({m_time_stamp, m_stack});
            m_stack.clear();
        }
        m_counter = 0;
        m_time_stamp = 0;
    };

   public:
    OFFSub(size_t N) : Cmd(), m_n(N) { m_stack.reserve(N); };
    ~OFFSub() { pub_and_clear(); };
    void callback(const command_t& in) override {
        if (!active.On(std::get<0>(in))) {
            if (active.Up()) {
                pub_and_clear();
            }
            return;
        }
        std::cout << "OFFSub\n";
        if (m_time_stamp == 0) {
            m_time_stamp = std::get<2>(in);
        }
        m_stack.emplace_back(std::move(std::get<1>(in)));
        m_counter++;
        if (m_counter == m_n) {
            pub_and_clear();
        }
    };
};

// блок
class ONSub : public Cmd {
   private:
    std::vector<std::string> m_stack;
    unix_time_stamp_t m_time_stamp{0};
    size_t m_n{3};
    size_t m_counter{0};
    Active<Level::FIRST> active{};
    int m_block_counter{};
    void clear() {
        if (!m_stack.empty()) {
            m_stack.clear();
        }
        m_counter = 0;
        m_time_stamp = 0;
    };
    void print() {
        if (!m_stack.empty()) {
            notify({m_time_stamp, m_stack});
        }
    };

   public:
    ONSub(size_t N) : Cmd(), m_n(N) { m_stack.reserve(N); };
    void callback(const command_t& in) override {
        if (!active.On(std::get<0>(in))) {
            if (active.Down()) {
                print();
                clear();
            }
            return;
        }
        std::cout << "ONSub\n";
        if (m_time_stamp == 0) {
            m_time_stamp = std::get<2>(in);
        }
        m_stack.emplace_back(std::move(std::get<1>(in)));
        m_counter++;
        if (m_counter > m_n) {
            clear();
        }
    };
};

// блок+
class INNERSub : public Cmd {
   private:
    size_t m_n{3};
    int m_current_level{0};
    int m_previous_level{0};
    struct Store {
        std::vector<std::string> stack{};
        unix_time_stamp_t time_stamp{0};
    };
    std::vector<Store> m_stack{};
    int m_stack_index{0};

   public:
    INNERSub(size_t N) : Cmd(), m_n(N) {};
    void callback(const command_t& in) override {
        auto status = std::get<0>(in);
        if (status == Status::BLOCK_ON) {
            m_previous_level = m_current_level;
            ++m_current_level;
        } else if (status == Status::BLOCK_OFF) {
            m_previous_level = m_current_level;
            --m_current_level;
        }
        if (m_current_level < 1) {
            return;
        }
        if (status != Status::NONE) {
            return;
        }
        std::cout << "INNERSub\n";
        if (m_current_level > m_previous_level) {  // Up
            ++m_stack_index;
            m_stack[m_stack_index];
        } else if (m_current_level < m_previous_level) {  // Down
            --m_stack_index;
        }

        // if (!active.On(std::get<0>(in))) {
        //     if (active.Down()) {
        //         // print();
        //         clear();
        //     }
        //     return;
        // }
        // if (m_time_stamp == 0) {
        //     m_time_stamp = std::get<2>(in);
        // }
        // m_stack.emplace_back(std::move(std::get<1>(in)));
        // m_counter++;
        // if (m_counter > m_n) {
        //     clear();
        // }
    };
};

// вывод на экран
class Display : public Subscriber<print_t> {
   public:
    Display() : Subscriber<print_t>() {};
    void callback(const print_t& in) override {
        std::cout << "bulk: ";
        auto it = in.second.begin();
        while (it != in.second.end()) {
            std::cout << *it;
            if (++it != in.second.end()) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    };
};
// вывод в файл
class File : public Subscriber<print_t> {
   public:
    File() : Subscriber<print_t>() {};
    void callback(const print_t& in) override {};
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " N" << std::endl;
        return 1;
    }
    size_t N = std::stoi(argv[1]);
    std::shared_ptr<Subscriber<print_t>> subs_print[] = {
        std::make_shared<Display>(),
        std::make_shared<File>(),
    };
    std::shared_ptr<Cmd> subs[] = {
        std::make_shared<OFFSub>(N),
        std::make_shared<ONSub>(N),
        std::make_shared<INNERSub>(N),
    };
    CinPub pub;
    for (auto v : subs) {
        for (auto vv : subs_print) {
            v->subscribe(vv);
        }
        pub.subscribe(v);
    };
    pub.run();
    return 0;
}
