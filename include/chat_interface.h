#ifndef CHAT_INTERFACE_H
#define CHAT_INTERFACE_H

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "command_manager.h"
#include "kolosal_server_client.h"

/**
 * @brief UI class for handling chat interface
 */
class ChatInterface {
public:
    /**
     * @brief Constructor
     * @param serverClient Shared pointer to the server client
     * @param commandManager Shared pointer to the command manager
     */
    ChatInterface(std::shared_ptr<KolosalServerClient> serverClient, 
                  std::shared_ptr<CommandManager> commandManager);
    
    /**
     * @brief Destructor
     */
    ~ChatInterface();
    
    /**
     * @brief Start an interactive chat session with the given model
     * @param engineId The engine ID to use for chat
     * @return True if chat session completed successfully, false if there was an error
     */
    bool startChatInterface(const std::string& engineId);

private:
    std::shared_ptr<KolosalServerClient> m_serverClient;
    std::shared_ptr<CommandManager> m_commandManager;
    
    /**
     * @brief Get user input with real-time command autocomplete
     * @param prompt The prompt to display
     * @return The complete user input string
     */
    std::string getInputWithRealTimeAutocomplete(const std::string& prompt);
    
    /**
     * @brief Update real-time command suggestions display
     * @param input Current input string
     * @param showingSuggestions Reference to flag tracking suggestion display state
     * @param suggestionStartRow Reference to row where suggestions start
     * @param prompt The input prompt
     */
    void updateRealTimeSuggestions(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt);
    
    /**
     * @brief Clear command suggestions from display
     * @param showingSuggestions Reference to flag tracking suggestion display state
     * @param suggestionStartRow Row where suggestions start
     * @param prompt The input prompt
     * @param input Current input string
     */
    void clearSuggestions(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input);
    
    /**
     * @brief Clear current input line for rewriting
     * @param input Current input string to clear
     * @param showingSuggestions Reference to flag tracking suggestion display state
     * @param suggestionStartRow Row where suggestions start
     * @param prompt The input prompt
     */
    void clearCurrentInput(const std::string& input, bool& showingSuggestions, int suggestionStartRow, const std::string& prompt);
    
    /**
     * @brief Force clear any lingering command suggestions from console
     */
    void forceClearSuggestions();
    
    /**
     * @brief Display hint text in the input area
     * @param hintText The hint text to display
     * @param showingHint Reference to bool tracking hint visibility
     */
    void displayHintText(const std::string& hintText, bool& showingHint);
    
    /**
     * @brief Clear hint text from the input area
     * @param hintText The hint text that was displayed
     * @param showingHint Reference to bool tracking hint visibility
     */
    void clearHintText(bool& showingHint);
    
    /**
     * @brief Display hint text in the input area (Linux implementation)
     * @param hintText The hint text to display
     * @param showingHint Reference to bool tracking hint visibility
     */
    void displayHintTextLinux(const std::string& hintText, bool& showingHint);
    
    /**
     * @brief Clear hint text from the input area (Linux implementation)
     * @param hintText The hint text that was displayed
     * @param showingHint Reference to bool tracking hint visibility
     */
    void clearHintTextLinux(const std::string& hintText, bool& showingHint);
    
    /**
     * @brief Update real-time suggestions for Linux
     * @param input Current input string
     * @param showingSuggestions Reference to bool tracking suggestion visibility
     * @param suggestionStartRow Starting row for suggestions
     * @param prompt Input prompt string
     */
    void updateRealTimeSuggestionsLinux(const std::string& input, bool& showingSuggestions, int& suggestionStartRow, const std::string& prompt);
    // Full-line redraw variant for Linux/mac: clears input & suggestions then redraws
    void redrawInputAndSuggestionsLinux(const std::string& input);
    
    /**
     * @brief Clear suggestions for Linux
     * @param showingSuggestions Reference to bool tracking suggestion visibility
     * @param suggestionStartRow Starting row for suggestions
     * @param prompt Input prompt string
     * @param input Current input string
     */
    void clearSuggestionsLinux(bool& showingSuggestions, int suggestionStartRow, const std::string& prompt, const std::string& input);
};

#endif // CHAT_INTERFACE_H
