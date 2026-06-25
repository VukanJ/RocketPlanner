#ifndef CMDARGS_H
#define CMDARGS_H

#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>

class CmdArgs {
public:
    CmdArgs(bool help_options = true);

    void add_option(const std::string& name, const std::string& description, bool requires_value = false);
    void add_flag(const std::string& name, const std::string& description);

    bool operator()(int argc, char** argv);

    std::optional<std::string> get(const std::string& name) const;
    bool is_flag_set(const std::string& name) const;

    void print_args() const;

private:
    void print_help() const;

    struct OptArg {
        enum Status : std::uint8_t {
            FLAG      = 0b1, // Is a flag (no value)
            FLAG_SET  = 0b10,
            VALUE_SET = 0b100,
            REQUIRED  = 0b1000,
            HELP      = 0b10000, // Help flag

            INIT_OPTION = 0b0000, // Optional argument that requires a value
        };
        std::uint8_t status = INIT_OPTION;

        inline bool is_flag()      const noexcept { return status & Status::FLAG; }
        inline bool is_mandatory() const noexcept { return status & Status::REQUIRED; }
        inline bool is_flag_set()  const noexcept { return status & Status::FLAG_SET; }
        inline bool is_value_set() const noexcept { return status & Status::VALUE_SET; }
        inline bool is_help()      const noexcept { return status & Status::HELP; }
        inline void set_flag()      noexcept { status |= Status::FLAG_SET; }
        inline void set_has_value() noexcept { status |= Status::VALUE_SET; }

        std::string description;
        std::optional<std::string> value;
    };

    std::string program_name;
    std::unordered_map<std::string, OptArg> options;
};

#endif
