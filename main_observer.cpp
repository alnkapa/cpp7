// sub-pub version
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <ranges>
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
            } else if (command == "}") {    // конец блока
                if (m_current_level > 0) {  // пропустить конец блока, если нет начала
                    notify({level(), Status::BLOCK_OFF, "", 0});
                    --m_current_level;
                }
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
    void callback(const print_t& in) override {
        auto it = in.second.begin();
        while (it != in.second.end()) {
            std::cout << *it;
            if (++it != in.second.end()) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    };
    void callback(const command_t& in) override {
        auto [level, status, cmd, time_stamp] = in;
        if (level != Level::NONE) {
            return;
        }
        // "cmd0" or "{"

        // "{"
        if (status == Status::BLOCK_ON) {
            pub_and_clear();
            return;
        }

        // cmd0
        if (status == Status::NONE) {
            if (m_time_stamp == 0) {
                m_time_stamp = time_stamp;
            }
            m_stack.emplace_back(std::move(cmd));
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
    void callback(const print_t& in) override {
        if (m_time_stamp == 0) {
            m_time_stamp = in.first;
        }
        for (auto v : in.second) {
            m_stack.emplace_back(std::move(v));
        }
    };
    void callback(const command_t& in) override {
        auto [level, status, cmd, time_stamp] = in;
        if (level != Level::FIRST) {
            return;
        }
        // cmd1  or "{" or "}"

        // "{"
        if (status == Status::BLOCK_ON) {
            return;
        }

        // "}"
        if (status == Status::BLOCK_OFF) {
            print();
            clear();
            return;
        }

        // cmd1
        if (status == Status::NONE) {
            if (m_time_stamp == 0) {
                m_time_stamp = time_stamp;
            }
            m_stack.emplace_back(std::move(cmd));
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
    std::map<size_t, std::tuple<std::vector<std::string>, unix_time_stamp_t, size_t>> m_level_map{};
    std::vector<std::string> m_curent_level_stack{};
    unix_time_stamp_t m_curent_level_time_stamp{};
    size_t m_curent_level_counter{0};
    int m_current_level{0};
    void clear() {
        if (!m_curent_level_stack.empty()) {
            m_curent_level_stack.clear();
        }
    };

   public:
    INNERSub(size_t N) : Cmd(), m_n(N) {};
    void callback(const print_t& in) override {};
    void callback(const command_t& in) override {
        auto [level, status, cmd, time_stamp] = in;
        if (level != Level::OTHER) {
            return;
        }
        // cmd1  or "{" or "}"

        // "{"
        if (status == Status::BLOCK_ON) {
            m_level_map[m_current_level] = std::move(
                std::make_tuple(std::move(m_curent_level_stack), m_curent_level_time_stamp, m_curent_level_counter));
            m_curent_level_time_stamp = 0;
            m_curent_level_counter = 0;
            ++m_current_level;
            return;
        }

        // "}"
        if (status == Status::BLOCK_OFF) {
            if (m_current_level == 0) {  // notify ONSub
                m_level_map[m_current_level] = std::move(std::make_tuple(
                    std::move(m_curent_level_stack), m_curent_level_time_stamp, m_curent_level_counter));
                m_curent_level_stack.clear();
                m_curent_level_time_stamp = 0;
                for (auto vv : m_level_map) {
                    auto [level_stack, level_time_stamp, level_counter] = vv.second;
                    if (!level_stack.empty()) {
                        if (m_curent_level_time_stamp == 0) {
                            m_curent_level_time_stamp = level_time_stamp;
                        }
                        for (auto v : level_stack) {
                            m_curent_level_stack.emplace_back(std::move(v));
                        }
                    }
                }
                if (!m_curent_level_stack.empty()) {
                    notify({m_curent_level_time_stamp, m_curent_level_stack});
                }
            } else {
                m_level_map[m_current_level] = std::move(std::make_tuple(
                    std::move(m_curent_level_stack), m_curent_level_time_stamp, m_curent_level_counter));
                if (m_current_level > 0) {
                    --m_current_level;
                }

                auto [level_stack, level_time_stamp, level_counter] = m_level_map[m_current_level];
                m_curent_level_stack = std::move(level_stack);
                m_curent_level_time_stamp = level_time_stamp;
                m_curent_level_counter = level_counter;
            }
            return;
        }

        // cmd1
        if (status == Status::NONE) {
            if (m_curent_level_time_stamp == 0) {
                m_curent_level_time_stamp = time_stamp;
            }
            m_curent_level_stack.emplace_back(std::move(cmd));
            m_curent_level_counter++;
            if (m_curent_level_counter > m_n) {
                clear();
            }
            return;
        }
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
    void callback(const print_t& in) override {
        std::ofstream file("bulk" + std::to_string(in.first) + ".log");
        if (file.is_open()) {
            file << "bulk: ";
            auto it = in.second.begin();
            while (it != in.second.end()) {
                file << *it;
                if (++it != in.second.end()) {
                    file << ", ";
                }
            }
            file << std::endl;
        }
    };
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
    auto [off, on, inner] = subs;
    inner->subscribe(on);
    for (auto v : subs_print) {
        off->subscribe(v);
        on->subscribe(v);
    }
    CinPub pub;
    for (auto v : subs) {
        pub.subscribe(v);
    };
    pub.run();

    return 0;
}
