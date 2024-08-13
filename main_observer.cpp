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
using command_t = std::tuple<Level, Status, std::string, unix_time_stamp_t>;
using print_t = std::pair<unix_time_stamp_t, std::vector<std::string>>;
class CinPub : public Publisher<command_t> {
   private:
    int m_current_level{0};
    Level level() {
        if (m_current_level == 0) {
            return Level::NONE;
        } else if (m_current_level == 1) {
            return Level::FIRST;
        } else {
            return Level::OTHER;
        }
    }

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
                notify({level(), Status::BLOCK_ON, "", 0});
                ++m_current_level;
            } else if (command == "}" && m_current_level > 0) {  // конец блока
                notify({level(), Status::BLOCK_OFF, "", 0});
                --m_current_level;
            } else {  // команда
                notify({level(), Status::NONE, command, time()});
            }
        }
    };
};

class Cmd : public Subscriber<command_t>, public Publisher<print_t>, public Subscriber<print_t> {};

// без блока
class OFFSub : public Cmd {
   private:
    std::vector<std::string> m_stack;
    unix_time_stamp_t m_time_stamp{0};
    size_t m_n{3};
    size_t m_counter{0};
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
    void callback(const print_t& in) override{

    };
    void callback(const command_t& in) override {
        if (std::get<0>(in) != Level::NONE) {
            return;
        }
        // "cmd0" or "{"

        // "{"
        if (std::get<1>(in) == Status::BLOCK_ON) {
            std::cout << "OFFSub\n";
            pub_and_clear();
            return;
        }

        // cmd0
        if (std::get<1>(in) == Status::NONE) {
            std::cout << "OFFSub\n";
            if (m_time_stamp == 0) {
                m_time_stamp = std::get<3>(in);
            }
            m_stack.emplace_back(std::move(std::get<2>(in)));
            m_counter++;
            if (m_counter == m_n) {
                pub_and_clear();
            }
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
    void callback(const print_t& in) override{

    };
    void callback(const command_t& in) override {
        if (std::get<0>(in) != Level::FIRST) {
            return;
        }
        // cmd1  or "{" or "}"

        // "{"
        if (std::get<1>(in) == Status::BLOCK_ON) {
            std::cout << "ONSub\n";
            // to INNER BOLck

            return;
        }

        // "}"
        if (std::get<1>(in) == Status::BLOCK_OFF) {
            std::cout << "ONSub\n";
            print();
            clear();
            return;
        }

        // cmd1
        if (std::get<1>(in) == Status::NONE) {
            std::cout << "ONSub\n";
            if (m_time_stamp == 0) {
                m_time_stamp = std::get<3>(in);
            }
            m_stack.emplace_back(std::move(std::get<2>(in)));
            m_counter++;
            if (m_counter > m_n) {
                clear();
            }
            return;
        }
    };
};

// блок+
class INNERSub : public Cmd {
   private:
    size_t m_n{3};
    struct Store {
        std::vector<std::string> stack{};
        unix_time_stamp_t time_stamp{0};
    };
    std::vector<Store> m_stack{};
    int m_stack_index{0};

   public:
    INNERSub(size_t N) : Cmd(), m_n(N){};
    void callback(const print_t& in) override{

    };
    void callback(const command_t& in) override {
        if (std::get<0>(in) != Level::OTHER) {
            return;
        }
        // cmd1  or "{" or "}"

        // "{"
        if (std::get<1>(in) == Status::BLOCK_ON) {
            std::cout << "INNERSub\n";
            // to INNERSub+
            return;
        }

        // "}"
        if (std::get<1>(in) == Status::BLOCK_OFF) {
            std::cout << "INNERSub\n";
            // to INNERSub+
            return;
        }

        // cmd1
        if (std::get<1>(in) == Status::NONE) {
            std::cout << "INNERSub\n";
            // if (m_time_stamp == 0) {
            //     m_time_stamp = std::get<3>(in);
            // }
            // m_stack.emplace_back(std::move(std::get<2>(in)));
            // m_counter++;
            // if (m_counter > m_n) {
            //     clear();
            // }
            return;
        }
    };
};

// вывод на экран
class Display : public Subscriber<print_t> {
   public:
    Display() : Subscriber<print_t>(){};
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
    File() : Subscriber<print_t>(){};
    void callback(const print_t& in) override{};
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
    subs[2]->subscribe(subs[1]);
    subs[1]->subscribe(subs[1]);
    for (auto v : subs_print) {
        subs[0]->subscribe(v);
        subs[1]->subscribe(v);
    }
    CinPub pub;
    for (auto v : subs) {
        pub.subscribe(v);
    };
    pub.run();
    return 0;
}
