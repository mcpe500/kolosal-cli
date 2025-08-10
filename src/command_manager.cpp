#include "command_manager.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <ctime>

CommandManager::CommandManager() : m_chatHistory(nullptr) {
    initializeBuiltInCommands();
}

bool CommandManager::isCommand(const std::string& input) const {
    return !input.empty() && input[0] == '/';
}

CommandResult CommandManager::executeCommand(const std::string& input) {
    if (!isCommand(input)) {
        return CommandResult(false, "Not a command");
    }
    
    auto [commandName, args] = parseCommand(input);
    
    if (commandName.empty()) {
        return CommandResult(false, "Invalid command format");
    }
    
    auto it = m_commands.find(commandName);
    if (it == m_commands.end()) {
        return CommandResult(false, "Unknown command: /" + commandName + "\nType /help to see available commands.");
    }
    
    try {
        return it->second.handler(args);
    } catch (const std::exception& e) {
        return CommandResult(false, "Error executing command: " + std::string(e.what()));
    }
}

void CommandManager::registerCommand(const std::string& name, const std::string& description, const std::string& usage, CommandHandler handler) {
    m_commands[name] = CommandInfo(name, description, usage, handler);
}

std::vector<CommandInfo> CommandManager::getAvailableCommands() const {
    std::vector<CommandInfo> commands;
    for (const auto& [name, info] : m_commands) {
        commands.push_back(info);
    }
    
    // Sort commands alphabetically
    std::sort(commands.begin(), commands.end(), 
        [](const CommandInfo& a, const CommandInfo& b) {
            return a.name < b.name;
        });
    
    return commands;
}

void CommandManager::setCurrentEngine(const std::string& engineId) {
    m_currentEngine = engineId;
}

void CommandManager::setChatHistory(std::vector<std::pair<std::string, std::string>>* history) {
    m_chatHistory = history;
}

std::pair<std::string, std::vector<std::string>> CommandManager::parseCommand(const std::string& input) const {
    if (input.empty() || input[0] != '/') {
        return {"", {}};
    }
    
    // Remove the leading '/'
    std::string command = input.substr(1);
    
    // Split by spaces
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) {
        return {"", {}};
    }
    
    std::string commandName = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    
    return {commandName, args};
}

void CommandManager::initializeBuiltInCommands() {
    // Help command
    registerCommand("help", "Show available commands", "/help [command]", 
        [this](const std::vector<std::string>& args) { return handleHelp(args); });
    
    // Exit command
    registerCommand("exit", "Exit the chat session", "/exit", 
        [this](const std::vector<std::string>& args) { return handleExit(args); });

    // JSON command
    registerCommand(
        "json",
        "Send next message with JSON constraints",
        "/json <json schema> (press Enter) then type your prompt",
        [this](const std::vector<std::string>& args) { return handleJson(args); }
    );
}

CommandResult CommandManager::handleHelp(const std::vector<std::string>& args) {
    std::ostringstream oss;
    
    if (args.empty()) {
        // Show all commands
        oss << "Available commands:\n\n";
        
        auto commands = getAvailableCommands();
        for (const auto& cmd : commands) {
            oss << std::left << std::setw(15) << ("/" + cmd.name) 
                << " - " << cmd.description << "\n";
        }
        
        oss << "\nType '/help <command>' for detailed usage of a specific command.";
    } else {
        // Show specific command help
        std::string commandName = args[0];
        auto it = m_commands.find(commandName);
        
        if (it == m_commands.end()) {
            return CommandResult(false, "Unknown command: /" + commandName);
        }
        
        const auto& cmd = it->second;
        oss << "Command: /" << cmd.name << "\n";
        oss << "Description: " << cmd.description << "\n";
        oss << "Usage: " << cmd.usage;
    }
    
    return CommandResult(true, oss.str());
}

CommandResult CommandManager::handleExit(const std::vector<std::string>& args) {
    return CommandResult(true, "Goodbye! 👋", true, false);
}

CommandResult CommandManager::handleJson(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult(false, "Usage: /json <json schema>\nExample: /json {\\\"type\\\":\\\"object\\\", \\\"properties\\\":{...}}", false, true);
    }

    // Join args back into a single schema string
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) oss << ' ';
        oss << args[i];
    }
    m_jsonSchema = oss.str();
    m_jsonPending = true;

    return CommandResult(true, "JSON mode enabled. Now type your prompt to generate a JSON that matches the schema.", false, true);
}

bool CommandManager::isPartialCommand(const std::string& input) const {
    return !input.empty() && input[0] == '/' && input.find(' ') == std::string::npos;
}

std::vector<std::string> CommandManager::getCommandSuggestions(const std::string& partialInput) const {
    std::vector<std::string> suggestions;
    
    if (partialInput.empty() || partialInput[0] != '/') {
        return suggestions;
    }
    
    // Get the command part without the '/'
    std::string prefix = partialInput.substr(1);
    
    // Find all commands that start with the prefix
    for (const auto& [commandName, commandInfo] : m_commands) {
        if (commandName.substr(0, prefix.length()) == prefix) {
            suggestions.push_back(commandName);
        }
    }
    
    // Sort suggestions alphabetically
    std::sort(suggestions.begin(), suggestions.end());
    
    return suggestions;
}

std::vector<std::string> CommandManager::getFormattedCommandSuggestions(const std::string& partialInput) const {
    std::vector<std::string> formatted;
    auto suggestions = getCommandSuggestions(partialInput);
    
    for (const std::string& commandName : suggestions) {
        auto it = m_commands.find(commandName);
        if (it != m_commands.end()) {
            std::string formatted_line = "/" + commandName;
            if (formatted_line.length() < 15) {
                formatted_line += std::string(15 - formatted_line.length(), ' ');
            }
            formatted_line += " - " + it->second.description;
            formatted.push_back(formatted_line);
        }
    }
    
    return formatted;
}
