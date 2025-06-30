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
    
    // Quit command (alias for exit)
    registerCommand("quit", "Exit the chat session", "/quit", 
        [this](const std::vector<std::string>& args) { return handleExit(args); });
    
    // Clear command
    registerCommand("clear", "Clear the chat history", "/clear", 
        [this](const std::vector<std::string>& args) { return handleClear(args); });
    
    // History command
    registerCommand("history", "Show chat history", "/history [count]", 
        [this](const std::vector<std::string>& args) { return handleHistory(args); });
    
    // Info command
    registerCommand("info", "Show current session information", "/info", 
        [this](const std::vector<std::string>& args) { return handleInfo(args); });
    
    // Save command
    registerCommand("save", "Save chat history to file", "/save [filename]", 
        [this](const std::vector<std::string>& args) { return handleSave(args); });
    
    // Load command
    registerCommand("load", "Load chat history from file", "/load <filename>", 
        [this](const std::vector<std::string>& args) { return handleLoad(args); });
    
    // Model command
    registerCommand("model", "Show current model information", "/model", 
        [this](const std::vector<std::string>& args) { return handleModel(args); });
    
    // Reset command
    registerCommand("reset", "Reset the conversation context", "/reset", 
        [this](const std::vector<std::string>& args) { return handleReset(args); });
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

CommandResult CommandManager::handleClear(const std::vector<std::string>& args) {
    if (m_chatHistory) {
        m_chatHistory->clear();
        return CommandResult(true, "Chat history cleared.");
    }
    return CommandResult(false, "Chat history not available.");
}

CommandResult CommandManager::handleHistory(const std::vector<std::string>& args) {
    if (!m_chatHistory) {
        return CommandResult(false, "Chat history not available.");
    }
    
    if (m_chatHistory->empty()) {
        return CommandResult(true, "No chat history available.");
    }
    
    // Determine how many messages to show
    int count = static_cast<int>(m_chatHistory->size());
    if (!args.empty()) {
        try {
            count = std::stoi(args[0]);
            count = std::max(1, std::min(count, static_cast<int>(m_chatHistory->size())));
        } catch (const std::exception&) {
            return CommandResult(false, "Invalid count. Please provide a valid number.");
        }
    }
    
    std::ostringstream oss;
    oss << "Chat History (last " << count << " messages):\n\n";
    
    // Show the last 'count' messages
    int start = std::max(0, static_cast<int>(m_chatHistory->size()) - count);
    for (int i = start; i < static_cast<int>(m_chatHistory->size()); ++i) {
        const auto& [role, content] = (*m_chatHistory)[i];
        oss << role << ":\n" << content << "\n\n";
    }
    
    return CommandResult(true, oss.str());
}

CommandResult CommandManager::handleInfo(const std::vector<std::string>& args) {
    std::ostringstream oss;
    
    oss << "Session Information:\n\n";
    oss << "Current Model: " << (m_currentEngine.empty() ? "Not set" : m_currentEngine) << "\n";
    
    if (m_chatHistory) {
        oss << "Messages in History: " << m_chatHistory->size() << "\n";
    } else {
        oss << "Messages in History: Not available\n";
    }
    
    // Get current timestamp
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    oss << "Current Time: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
    
    return CommandResult(true, oss.str());
}

CommandResult CommandManager::handleSave(const std::vector<std::string>& args) {
    if (!m_chatHistory) {
        return CommandResult(false, "Chat history not available.");
    }
    
    if (m_chatHistory->empty()) {
        return CommandResult(false, "No chat history to save.");
    }
    
    // Generate filename if not provided
    std::string filename;
    if (args.empty()) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << "chat_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".txt";
        filename = oss.str();
    } else {
        filename = args[0];
        // Add .txt extension if not present
        if (filename.find('.') == std::string::npos) {
            filename += ".txt";
        }
    }
    
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return CommandResult(false, "Failed to create file: " + filename);
        }
        
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        file << "Chat History Export\n";
        file << "Model: " << m_currentEngine << "\n";
        file << "Date: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
        file << std::string(50, '=') << "\n\n";
        
        for (const auto& [role, content] : *m_chatHistory) {
            file << role << ":\n" << content << "\n\n";
        }
        
        file.close();
        return CommandResult(true, "Chat history saved to: " + filename);
    } catch (const std::exception& e) {
        return CommandResult(false, "Error saving file: " + std::string(e.what()));
    }
}

CommandResult CommandManager::handleLoad(const std::vector<std::string>& args) {
    if (args.empty()) {
        return CommandResult(false, "Please specify a filename. Usage: /load <filename>");
    }
    
    std::string filename = args[0];
    if (filename.find('.') == std::string::npos) {
        filename += ".txt";
    }
    
    if (!std::filesystem::exists(filename)) {
        return CommandResult(false, "File not found: " + filename);
    }
    
    if (!m_chatHistory) {
        return CommandResult(false, "Chat history not available.");
    }
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return CommandResult(false, "Failed to open file: " + filename);
        }
        
        // Clear current history
        m_chatHistory->clear();
        
        std::string line;
        std::string currentRole;
        std::string currentContent;
        bool inContent = false;
        
        while (std::getline(file, line)) {
            // Skip header lines
            if (line.find("Chat History Export") != std::string::npos ||
                line.find("Model:") != std::string::npos ||
                line.find("Date:") != std::string::npos ||
                line == std::string(50, '=')) {
                continue;
            }
            
            // Check if this is a role line (ends with ':')
            if (!line.empty() && line.back() == ':' && 
                (line.find("user:") == 0 || line.find("assistant:") == 0)) {
                
                // Save previous message if we have one
                if (!currentRole.empty() && !currentContent.empty()) {
                    m_chatHistory->push_back({currentRole, currentContent});
                }
                
                // Start new message
                currentRole = line.substr(0, line.length() - 1);
                currentContent.clear();
                inContent = true;
            } else if (inContent) {
                if (line.empty()) {
                    // Empty line might indicate end of message
                    if (!currentContent.empty()) {
                        // Remove trailing newline if present
                        if (currentContent.back() == '\n') {
                            currentContent.pop_back();
                        }
                    }
                } else {
                    currentContent += line + "\n";
                }
            }
        }
        
        // Add the last message
        if (!currentRole.empty() && !currentContent.empty()) {
            if (currentContent.back() == '\n') {
                currentContent.pop_back();
            }
            m_chatHistory->push_back({currentRole, currentContent});
        }
        
        file.close();
        return CommandResult(true, "Chat history loaded from: " + filename + 
                           " (" + std::to_string(m_chatHistory->size()) + " messages)");
    } catch (const std::exception& e) {
        return CommandResult(false, "Error loading file: " + std::string(e.what()));
    }
}

CommandResult CommandManager::handleModel(const std::vector<std::string>& args) {
    std::ostringstream oss;
    
    oss << "Current Model Information:\n\n";
    oss << "Engine ID: " << (m_currentEngine.empty() ? "Not set" : m_currentEngine) << "\n";
    
    // TODO: Add more model information if available (model file path, parameters, etc.)
    
    return CommandResult(true, oss.str());
}

CommandResult CommandManager::handleReset(const std::vector<std::string>& args) {
    if (m_chatHistory) {
        m_chatHistory->clear();
        return CommandResult(true, "Conversation context reset. Starting fresh!");
    }
    return CommandResult(false, "Chat history not available.");
}
