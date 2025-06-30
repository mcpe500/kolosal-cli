#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <memory>

/**
 * @brief Result of command execution
 */
struct CommandResult {
    bool success;
    std::string message;
    bool shouldExit;
    bool shouldContinueChat;
    
    CommandResult(bool success = true, const std::string& message = "", bool shouldExit = false, bool shouldContinueChat = true)
        : success(success), message(message), shouldExit(shouldExit), shouldContinueChat(shouldContinueChat) {}
};

/**
 * @brief Command handler function type
 */
using CommandHandler = std::function<CommandResult(const std::vector<std::string>&)>;

/**
 * @brief Information about a command
 */
struct CommandInfo {
    std::string name;
    std::string description;
    std::string usage;
    CommandHandler handler;
    
    // Default constructor
    CommandInfo() = default;
    
    CommandInfo(const std::string& name, const std::string& description, const std::string& usage, CommandHandler handler)
        : name(name), description(description), usage(usage), handler(handler) {}
};

/**
 * @brief Command manager for handling chat commands starting with "/"
 */
class CommandManager {
public:
    /**
     * @brief Constructor
     */
    CommandManager();
    
    /**
     * @brief Check if input is a command (starts with "/")
     * @param input The user input
     * @return True if it's a command, false otherwise
     */
    bool isCommand(const std::string& input) const;
    
    /**
     * @brief Parse and execute a command
     * @param input The command input starting with "/"
     * @return CommandResult indicating success/failure and any message
     */
    CommandResult executeCommand(const std::string& input);
    
    /**
     * @brief Register a new command
     * @param name Command name (without the "/")
     * @param description Brief description of the command
     * @param usage Usage syntax
     * @param handler Function to handle the command
     */
    void registerCommand(const std::string& name, const std::string& description, const std::string& usage, CommandHandler handler);
    
    /**
     * @brief Get list of all available commands
     * @return Vector of command information
     */
    std::vector<CommandInfo> getAvailableCommands() const;
    
    /**
     * @brief Set the current engine ID for commands that need it
     * @param engineId The current engine/model ID
     */
    void setCurrentEngine(const std::string& engineId);
    
    /**
     * @brief Set chat history for commands that need access to it
     * @param history Reference to chat history
     */
    void setChatHistory(std::vector<std::pair<std::string, std::string>>* history);

private:
    std::map<std::string, CommandInfo> m_commands;
    std::string m_currentEngine;
    std::vector<std::pair<std::string, std::string>>* m_chatHistory;
    
    /**
     * @brief Parse command input into command name and arguments
     * @param input The command input
     * @return Pair of command name and arguments vector
     */
    std::pair<std::string, std::vector<std::string>> parseCommand(const std::string& input) const;
    
    /**
     * @brief Initialize built-in commands
     */
    void initializeBuiltInCommands();
    
    // Built-in command handlers
    CommandResult handleHelp(const std::vector<std::string>& args);
    CommandResult handleExit(const std::vector<std::string>& args);
    CommandResult handleClear(const std::vector<std::string>& args);
    CommandResult handleHistory(const std::vector<std::string>& args);
    CommandResult handleInfo(const std::vector<std::string>& args);
    CommandResult handleSave(const std::vector<std::string>& args);
    CommandResult handleLoad(const std::vector<std::string>& args);
    CommandResult handleModel(const std::vector<std::string>& args);
    CommandResult handleReset(const std::vector<std::string>& args);
};

#endif // COMMAND_MANAGER_H
