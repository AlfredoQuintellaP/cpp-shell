#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

// Convenções comuns 
// return 0;   // Sucesso
// return 1;   // Erro geral
// return 2;   // Uso incorreto do comando  
// return 127; // Comando não encontrado
// return 130; // Processo terminado com Ctrl+C

#include <cstdlib> 
const char* path_env = std::getenv("PATH");

#ifdef _WIN32
    const char PATH_SEPARATOR = ';';
#else
    const char PATH_SEPARATOR = ':';
#endif

std::unordered_map<std::string, std::function<int(const std::vector<std::string>&)>> builtins;

std::vector<std::string> split_command(const std::string& input) {
    // STATES:
    // 1. NORMAL (default) - Outside quotes
    //    - Spaces break tokens
    //    - Backslash (\) enters ESCAPING state
    //    - Quotes enter respective quote states
    //
    // 2. IN_QUOTES_SINGLE - Inside ' quotes
    //    - EVERYTHING IS LITERAL - no special characters
    //    - Single quote (') returns to NORMAL
    //    - Backslash (\) treated as normal character
    //    - Double quotes (") treated as normal characters
    //
    // 3. IN_QUOTES_DOUBLE - Inside " quotes
    //    - Limited escaping supported
    //    - Double quote (") returns to NORMAL
    //    - Backslash (\) enters ESCAPING state
    //    - Single quotes (') treated as normal characters
    //
    // 4. ESCAPING (temporary) - Processes escaped character
    //    - Duration: Exactly one character
    //    - Behavior depends on previous state
    //
    // STATE TRANSITIONS:
    // NORMAL + '  - IN_QUOTES_SINGLE
    // NORMAL + "  - IN_QUOTES_DOUBLE
    // NORMAL + \  - ESCAPING
    // NORMAL + space - Break token (stay NORMAL)
    //
    // IN_QUOTES_SINGLE + '  - NORMAL
    // IN_QUOTES_SINGLE + other - Add to token (stay)
    //
    // IN_QUOTES_DOUBLE + "  - NORMAL
    // IN_QUOTES_DOUBLE + \  - ESCAPING
    // IN_QUOTES_DOUBLE + other - Add to token (stay)
    //
    // ESCAPING transitions (based on previous state):
    // Previous: NORMAL - Add character - NORMAL
    // Previous: IN_QUOTES_DOUBLE:
    //   + " or \ - Add character - IN_QUOTES_DOUBLE
    //   + other - Add \ + character - IN_QUOTES_DOUBLE
    //
    // ESCAPE BEHAVIOR BY CONTEXT:
    // Outside quotes (NORMAL): \ + any_char - any_char (backslash removed)
    // Single quotes: \ - \ (literal, no escape)
    // Double quotes: 
    //   \" - " (escaped quote)
    //   \\ - \ (escaped backslash)
    //   \ + other - \ + other (backslash preserved)
    //
    // TOKEN BREAKING:
    // - Only breaks by spaces in NORMAL state
    // - Spaces inside quotes become part of token
    // - Consecutive NORMAL spaces collapse to single break
    // - Empty quotes don't create empty tokens
    
    std::vector<std::string> tokens;
    std::string curr_token;

    char curr_quote = '\0';
    bool in_quotes = false;
    bool escaping = false;

    for (char c : input) {
        if (escaping) {
            if (in_quotes && curr_quote == '"') {
                if (c == '"' || c == '\\') {
                    curr_token += c;  
                } else {
                    curr_token += '\\';
                    curr_token += c;
                }
            } else {
                curr_token += c;
            }
            escaping = false;
        }
        else if (c == '\\') {
            if (in_quotes && curr_quote == '\'') {
                curr_token += c;
            } else {
                escaping = true;
            }
        }
        else if (!in_quotes && (c == '"' || c == '\'')) {
            in_quotes = true;
            curr_quote = c;
        }
        else if (in_quotes && c == curr_quote) {
            in_quotes = false;
            curr_quote = '\0';
        }
        else if (std::isspace(c) && !in_quotes) {
            if (!curr_token.empty()) {
                tokens.push_back(curr_token);
                curr_token.clear();
            }
        }
        else {
            curr_token += c;
        }
    }
    
    if (escaping) {
        curr_token += '\\';
    }
    
    if (!curr_token.empty()) {
        tokens.push_back(curr_token);
    }
    
    return tokens;
}

std::vector<std::string> split_path(const std::string& path_env) {
    std::vector<std::string> directories;
    std::stringstream ss(path_env);
    std::string directory;

    // Dividindo pelo separator definido
    while(std::getline(ss, directory, PATH_SEPARATOR)) {
        if(!directory.empty()) {
            directories.push_back(directory);
        }
    }

    return directories;
}

// Solving for non-builtin functions
// See stat struct again later, it seems really good and dont seems to have
// the access() problem...
// Basicamente, vamos ver os file mode bits e ver como esta o bit pra permissao
// Queremos que o bit de execucao esteja ligado, isto eh, o and com S_IXUSR vai estar
// true
// Alem disso, o arquivo precisa ser regular, logo S_IFREG() é true
bool is_executable(const std::string& filepath) {
    struct stat statbuf;
    if(stat(filepath.c_str(), &statbuf) == 0) {
        return S_ISREG(statbuf.st_mode) && (statbuf.st_mode & S_IXUSR);
    }
    return false;
}

std::string find_executable_in_path(const std::string& command) {
    const char* path_env = std::getenv("PATH");
    if(!path_env) {
        return "";
    }

    std::vector<std::string> path_dirs = split_path(path_env);

    for(const auto& directory : path_dirs) {
        std::string full_path = directory + "/" + command;

        if(is_executable(full_path)) {
            return full_path;
        }
    }

    return "";
}

int exit_command(const std::vector<std::string>& args) {
    exit(0);
    return 0;
}

int echo_command(const std::vector<std::string>& args) {
    for(std::string arg : args) {
        std::cout << arg << " ";
    }
    std::cout << "\n";
    return 0;
}

int type_command(const std::vector<std::string>& args) {
    if(args.empty()) {
        std::cout << "error";
    }

    auto it = builtins.find(args[0]);
    if(it != builtins.end()) {
        std::cout << args[0] << " is a shell builtin\n";
        return 0;
    }

    std::string cmd_path = find_executable_in_path(args[0]);
    if(cmd_path != "") {
        std::cout << args[0] << " is " << cmd_path << "\n";
        return 0;
    } else {
        std::cout << args[0] << ": not found\n";
    }

    return 0;
}

int execute_external_command(const std::string& cmd_path, const std::vector<std::string>& args) {
    pid_t pid = fork();

    //executando apenas no processo filho
    if(pid == 0) {
        std::vector<char*> exec_args;
        std::string cmd_name = cmd_path.substr(cmd_path.find_last_of("/") + 1);
        exec_args.push_back(const_cast<char*>(cmd_name.c_str()));

        for(const auto& arg : args) {
            exec_args.push_back(const_cast<char*>(arg.c_str()));
        }
        exec_args.push_back(nullptr);

        execvp(cmd_path.c_str(), exec_args.data());

        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        //executando processo pai
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        //Fork falhou
        perror("fork failed");
        return 1;
    }
}

int execute_command(const std::string& cmd_name, const std::vector<std::string>& args) {
    auto it = builtins.find(cmd_name);
    if(it != builtins.end()) {
        return it->second(args); 
    }

    std::string cmd_path = find_executable_in_path(cmd_name);
    if(!cmd_path.empty()) {
        return execute_external_command(cmd_path, args);
    }
    

    std::cout << cmd_name << ": command not found" << std::endl;
    return 127; // command not found
}

int main() {
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    //INITIALIZIN THE MAP FOR BUILT-IN COMMANDS
    builtins["exit"] = exit_command;
    builtins["echo"] = echo_command;
    builtins["type"] = type_command;

    while(1){
        std::cout << "$ ";

        std::string command;
        std::getline(std::cin, command);

        if(command.empty()) {
            continue;
        }

        std::vector<std::string> args = split_command(command);
        std::string cmd = args[0];
        args.erase(args.begin());

        execute_command(cmd, args);
    }
    return 0;
}
