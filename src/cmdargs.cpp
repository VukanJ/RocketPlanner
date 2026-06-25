#include "cmdargs.h"

#include <print>

CmdArgs::CmdArgs(bool help_options) {
    if (help_options) {
        options["-h"] = OptArg{static_cast<OptArg::Status>(OptArg::Status::HELP | OptArg::Status::FLAG), "Help", std::nullopt };
        options["--help"] = options["-h"]; 
    }
}

void CmdArgs::add_option(const std::string& name, const std::string& description, bool mandatory) {
    options[name] = OptArg{ mandatory ? OptArg::Status::REQUIRED : OptArg::Status::INIT_OPTION, description, std::nullopt };
}

void CmdArgs::add_flag(const std::string& name, const std::string& description) {
    options[name] = OptArg{ OptArg::Status::FLAG, description, std::nullopt };
}

bool CmdArgs::operator()(int argc, char** argv) {
    program_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (options.find(arg) != options.end()) {
            OptArg& opt = options[arg];
            if (opt.is_flag()) {
                opt.set_flag();
            }
            else {
                if (i + 1 < argc) {
                    opt.value = argv[i + 1];
                    opt.set_has_value();
                    ++i;
                }
                else {
                    return false;
                }
            }
        }
        else {
            return false;
        }
    }
    
    for (const auto& [name, opt] : options) {
        if (opt.is_mandatory() && !opt.is_value_set()) {
            return false;
        }
        if (opt.is_help() && opt.is_flag_set()) {
            print_help();
            return true;
        }
    }
    return true;
}

std::optional<std::string> CmdArgs::get(const std::string& name) const {
    auto it = options.find(name);
    if (it != options.end() && it->second.is_value_set()) {
        return it->second.value;
    }
    return std::nullopt;
}

bool CmdArgs::is_flag_set(const std::string& name) const {
    auto it = options.find(name);
    if (it != options.end() && it->second.is_flag()) {
        return it->second.is_flag_set();
    }
    return false;
}


void CmdArgs::print_args() const {
    std::println("Command-line arguments:");
    for (const auto& [name, arg] : options) {
        if (arg.is_help()) continue; // Skip help itself
        if (arg.is_flag()) {
            std::println("  \"{}\": \t Flag, set: {}", name, arg.is_flag_set() ? "true" : "false");
        } else {
            std::println("  \"{}\" <value>: \t {}{}, value: {}", name, arg.description, arg.is_mandatory() ? " (mandatory)" : "", arg.is_value_set() ? *arg.value : "not set");
        }
    }
}

void CmdArgs::print_help() const {
    std::println("Available command-line options for executable {}:", program_name);
    for (const auto& [name, arg] : options) {
        if (arg.is_help()) continue; // Skip help itself
        if (arg.is_flag()) {
            std::println("  \"{}\": \t Flag ({})", name, arg.description);
        } else {
            std::println("  \"{}\" <value>: \t {}{}", name, arg.description, arg.is_mandatory() ? " (mandatory)" : "");
        }
    }
}
