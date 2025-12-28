#pragma once

#include <conio.h>

#include <Common.hpp>
#include <GUIWin.hpp>
#include <Args.hpp>

class Shell {
    struct Command {
        std::string name;
        std::function<void(const std::vector<std::string>&)> function;
        std::string description;
    };

    std::vector<Command> commands;

public:
    std::string prompt;
    bool running;

    explicit Shell(std::string  promptText = "Shell") :
        prompt(std::move(promptText)), running(true) {
        setupBaseCommands();
    }

    template<typename... Args>
    void registerCommand(
        const std::string& name,
        const std::function<void(const std::vector<std::string>&)> &func,
        const std::string& desc = ""
    ) {
        std::string fullDesc = desc;
        if constexpr(sizeof...(Args) > 0) {
            if (!fullDesc.empty())
                fullDesc += " ";
            fullDesc += YELLOW + "\n Args: ";
            fullDesc += buildArgList<Args...>();
            fullDesc += RES;
        }

        auto wrapper = [func](const std::vector<std::string> &args) -> void {
            constexpr std::size_t N = sizeof...(Args);
            if (args.size() > N) {
                std::cerr << "Error: too many arguments (max " << N << ")\n";
                return;
            }
            func(args);
        };

        commands.push_back({name, func, fullDesc});
    }

    //& Regular member functions with args.
    template<typename T, typename Ret, typename... Args>
    void registerMemberFn(
        const std::string &name,
        T *obj,
        Ret (T::*method)(Args...),
        const std::string &description = ""
    ) {
        registerCommand<Args...>(name, [obj, method](const std::vector<std::string> &args) {
            auto parsed = MakeArgs<Args...>(std::make_index_sequence<sizeof...(Args)>{}, args);
            std::apply([&](auto &&... unpacked) {
                (obj->*method)(unpacked...);
            }, parsed);
        }, description);
    }

    //& Const member functions with args.
    template<typename T, typename Ret, typename... Args>
    void registerMemberFn(
        const std::string &name,
        T *obj, Ret (T::*method)(Args...) const,
        const std::string &description = ""
    ) {
        registerCommand<Args...>(name, [obj, method](const std::vector<std::string> &args) {
            auto parsed = MakeArgs<Args...>(std::make_index_sequence<sizeof...(Args)>{}, args);
            std::apply([&](auto &&... unpacked) {
                (obj->*method)(unpacked...);
            }, parsed);
        }, description);
    }

    //& Static functions with args.
    template<typename Ret, typename... Args>
    void registerMemberFn(
        const std::string &name,
        Ret (*method)(Args...),
        const std::string &description = ""
    ) {
        registerCommand<Args...>(name, [method](const std::vector<std::string> &args) {
            auto parsed = MakeArgs<Args...>(std::make_index_sequence<sizeof...(Args)>{}, args);
            std::apply([&](auto &&... unpacked) {
                method(unpacked...);
            }, parsed);
        }, description);
    }

    //& Const member function with no args.
    template<typename T>
    void registerMemberFn(
        const std::string& name,
        T* obj, void (T::*method)() const,
        const std::string& description = ""
    ) {
        registerCommand(name, [obj, method](const std::vector<std::string>&) -> void {
            (obj->*method)();
        }, description);
    }

    void setPrompt(const std::string& promptText) {
        prompt = promptText;
    }

    void run() const {
        while (running) {
            std::cout << prompt << "> ";
            std::cout.flush();

            std::string input = getInput();

            if (input.empty()) continue;

            executeCommand(input);
        }
    }

    void executeCommand(std::string& input) const {
        std::ranges::transform(input, input.begin(),
            [](const unsigned char c) -> uint8_t {return std::tolower(c);});

        std::vector<std::string> tokens = tokenize(input);
        if (tokens.empty()) return;
        const std::string commandName = tokens[0];
        for (int i = 0; i < tokens.size() - 1; ++i) {
            tokens[i] = tokens[i + 1];
        }

        tokens.pop_back();

        for (const auto& cmd : commands) {
            if (cmd.name == commandName) {
                std::thread([&cmd, tokens]() -> void {
                    cmd.function(tokens);
                }).detach();
                return;
            }
        }

        std::cout << "Unknown command: " << commandName << ". Type 'help' for available commands.\n";
    }

    void stop() {
        running = false;
    }

    static std::string getSimpleInput() {
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

private:
    void setupBaseCommands() {
        registerCommand("help", [this](const std::vector<std::string>& args) -> void {
            showHelp();
        }, "Show available commands");
    }

    static std::string getInput() {
        std::vector<char> buffer;
        std::string originalInput;

        while (true) {
            if (const int c = _getch(); c == 13) { // Enter
                std::cout << "\n";
                break;
            } else if (c == 8) { // Backspace
                if (!buffer.empty()) {
                    buffer.pop_back();
                    std::cout << "\b \b";
                }
            } else if (c >= 32 && c <= 126) {
                buffer.push_back(c);
                std::cout << static_cast<char>(c);
            }
        }

        return std::string(buffer.begin(), buffer.end());
    }

    static std::vector<std::string> tokenize(const std::string& input) {
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        std::string token;

        while (ss >> token) {
            tokens.push_back(token);
        }

        return tokens;
    }

    void showHelp() const {
        std::cout << "Commands:\n";
        std::cout << "==================\n";

        size_t maxLen = 0;
        for (const auto& cmd : commands) {
            maxLen = std::max(maxLen, cmd.name.length());
        }

        std::vector<Command> sortedCommands = commands;
        std::ranges::sort(sortedCommands,
                          [](const Command& a, const Command& b) {
                              return a.name < b.name;
                          });

        for (const auto& cmd : sortedCommands) {
            std::cout << GREEN << cmd.name << RES;

            for (size_t i = cmd.name.length(); i < maxLen + 2; ++i) {
                std::cout << " ";
            }

            if (!cmd.description.empty()) {
                std::cout << "- " << cmd.description;
            }
            std::cout << "\n\n";
        }
    }
};

// *** IMGUI -----------------------------------------------------------------------------------------------------
#include <Ansi.hpp>

const std::string COL = "\033[36m";

class TerminalBuffer final : public std::streambuf {
public:
    std::ostringstream buffer;
    std::mutex mutex;

protected:
    int overflow(const int c) override {
        std::lock_guard lock(mutex);
        buffer.put(static_cast<char>(c));
        return c;
    }
};

class ImGuiTerminalShell {
    TerminalBuffer      outBuf;
    char                inputLine[256];
    std::string         outputText;
    bool                scrollToBottom = true;
public:
    Shell               shell;
    GuiHooks::HookId    Hook;

    ImGuiTerminalShell()
        : inputLine{}, shell(COL + "IBAT" + RES), Hook(0) {
        shell.registerCommand("clear", [this](const std::vector<std::string> &) {
            {
                std::lock_guard<std::mutex> lock(outBuf.mutex);
                outBuf.buffer.str("");
                outBuf.buffer.clear();
            }
            std::cout << shell.prompt << "> \n";
            std::cout.flush();
            this->scrollToBottom = true;
        }, "Clear the screen");

        shell.registerCommand("exit", [this](const std::vector<std::string> &args) {
            shell.running = false;
            GuiHooks::unregisterRenderCallback(Hook);
        }, "Exit the shell");

        shell.registerCommand("quit", [this](const std::vector<std::string> &args) {
            shell.running = false;
            GuiHooks::unregisterRenderCallback(Hook);
        }, "Exit the shell");
    }

    void Init() {
        std::cout.rdbuf(&outBuf);
        std::cerr.rdbuf(&outBuf);
    }

void Render() {
        {
            std::lock_guard lock(outBuf.mutex);
            outputText = outBuf.buffer.str();
        }

        const ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0,0),   ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_HorizontalScrollbar;

        if (ImGui::Begin("Shell", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            {
                if (ImGui::BeginChild("Out", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()*2), false, flags)) {
                    const float current_scroll = ImGui::GetScrollY();
                    const float max_scroll     = ImGui::GetScrollMaxY();
                    const bool at_bottom = (max_scroll <= 0.0f) || (current_scroll >= max_scroll - 1.0f);

                    ImGuiAnsiText(outputText);

                    if (scrollToBottom) {
                        ImGui::SetScrollHereY(1.0f);
                        scrollToBottom = false;
                    }
                    if (at_bottom)
                        ImGui::SetScrollHereY(1.0f);
                }

                ImGui::EndChild();
            }

            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 40, 40));
            if (ImGui::Button("⭳", ImVec2(-1, 0)))
                scrollToBottom = true;
            ImGui::PopStyleColor();

            ImGui::AlignTextToFramePadding();
            ImGuiAnsiText(COL + "IBAT" + RES + ">");

            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);

            static bool focusInput = false;

            const bool submitted = ImGui::InputText("##Input", inputLine, sizeof(inputLine),
                                              ImGuiInputTextFlags_EnterReturnsTrue);


            if (submitted) {
                std::cout << shell.prompt << "> " << inputLine << "\n";
                if (shell.running) {
                    std::string input = inputLine;
                    shell.executeCommand(input);
                }

                inputLine[0] = '\0';
                focusInput = true;
            }

            if (focusInput) {
                ImGui::SetKeyboardFocusHere(-1);
                focusInput = false;
            }
        }
        ImGui::End();

    }
};
