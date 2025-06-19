#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <windows.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iomanip>

using json = nlohmann::json;

// Callback function to write response data
struct HttpResponse {
    std::string data;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    HttpResponse* response = static_cast<HttpResponse*>(userp);    response->data.append(static_cast<char*>(contents), realsize);
    return realsize;
}

struct QuantizationInfo {
    std::string type;
    std::string description;
    int priority; // Lower number = higher priority for default selection
};

// Function to detect quantization type and provide description
QuantizationInfo detectQuantization(const std::string& filename) {
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    
    // Check for various quantization patterns
    if (lower_filename.find("q8_0") != std::string::npos) {
        return {"Q8_0", "8-bit quantization, good balance of quality and size", 1};
    } else if (lower_filename.find("q6_k") != std::string::npos) {
        return {"Q6_K", "6-bit quantization, high quality with smaller size", 2};
    } else if (lower_filename.find("q5_k_m") != std::string::npos) {
        return {"Q5_K_M", "5-bit quantization (medium), good quality/size balance", 3};
    } else if (lower_filename.find("q5_k_s") != std::string::npos) {
        return {"Q5_K_S", "5-bit quantization (small), smaller size", 4};
    } else if (lower_filename.find("q5_0") != std::string::npos) {
        return {"Q5_0", "5-bit quantization, legacy format", 5};
    } else if (lower_filename.find("q4_k_m") != std::string::npos) {
        return {"Q4_K_M", "4-bit quantization (medium), good for most use cases", 6};
    } else if (lower_filename.find("q4_k_s") != std::string::npos) {
        return {"Q4_K_S", "4-bit quantization (small), very compact", 7};
    } else if (lower_filename.find("q4_0") != std::string::npos) {
        return {"Q4_0", "4-bit quantization, legacy format", 8};
    } else if (lower_filename.find("q3_k_l") != std::string::npos) {
        return {"Q3_K_L", "3-bit quantization (large), experimental", 9};
    } else if (lower_filename.find("q3_k_m") != std::string::npos) {
        return {"Q3_K_M", "3-bit quantization (medium), very small size", 10};
    } else if (lower_filename.find("q3_k_s") != std::string::npos) {
        return {"Q3_K_S", "3-bit quantization (small), ultra compact", 11};
    } else if (lower_filename.find("q2_k") != std::string::npos) {
        return {"Q2_K", "2-bit quantization, extremely small but lower quality", 12};
    } else if (lower_filename.find("f16") != std::string::npos) {
        return {"F16", "16-bit floating point, highest quality but large size", 13};
    } else if (lower_filename.find("f32") != std::string::npos) {
        return {"F32", "32-bit floating point, original precision", 14};
    } else {
        return {"Unknown", "Unknown quantization type", 15};
    }
}

struct ModelFile {
    std::string filename;
    QuantizationInfo quant;
    
    std::string getDisplayName() const {
        return filename + " (" + quant.type + ": " + quant.description + ")";
    }
};

// HTTP client for Hugging Face API using curl
class HuggingFaceClient {
public:
    static std::vector<std::string> fetchKolosalModels() {
        std::vector<std::string> models;
          std::cout << "Initializing HTTP client...\n";
        
        // Initialize curl
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize curl" << std::endl;
            return models;
        }
        
        HttpResponse response;
          // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, "https://huggingface.co/api/models?search=kolosal&limit=50");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // Disable SSL verification for testing
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);  // Disable hostname verification for testing
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return models;
        }
          // Check HTTP status code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            std::cerr << "HTTP request failed with status code: " << response_code << std::endl;
            curl_easy_cleanup(curl);
            return models;
        }
        
        curl_easy_cleanup(curl);
        
        // Parse JSON response
        try {
            json jsonData = json::parse(response.data);
            
            // Extract model IDs
            if (jsonData.is_array()) {
                for (const auto& model : jsonData) {
                    if (model.contains("id") && model["id"].is_string()) {
                        std::string modelId = model["id"];
                        // Only include models from kolosal organization
                        if (modelId.find("kolosal/") == 0) {
                            models.push_back(modelId);
                        }
                    }                }
            }
            
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
            return models;
        }
        
        return models;
    }    static std::vector<ModelFile> fetchModelFilesWithQuant(const std::string& modelId) {
        std::vector<ModelFile> modelFiles;
        std::cout << "Fetching files for model: " << modelId << "...\n";
        
        // Initialize curl
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize curl" << std::endl;
            return modelFiles;
        }
        
        HttpResponse response;
        
        // Construct API URL for model files
        std::string url = "https://huggingface.co/api/models/" + modelId + "/tree/main";
        
        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kolosal-CLI/1.0");
        
        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return modelFiles;
        }
        
        // Check HTTP status code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            std::cerr << "HTTP request failed with status code: " << response_code << std::endl;
            curl_easy_cleanup(curl);
            return modelFiles;
        }
        
        curl_easy_cleanup(curl);
        
        // Parse JSON response
        try {
            json jsonData = json::parse(response.data);
            
            // Extract .gguf files
            if (jsonData.is_array()) {
                for (const auto& item : jsonData) {
                    if (item.contains("type") && item["type"].is_string() && 
                        item["type"] == "file" && item.contains("path") && item["path"].is_string()) {
                        std::string filename = item["path"];
                        // Check if file has .gguf extension
                        if (filename.length() >= 5 && filename.substr(filename.length() - 5) == ".gguf") {
                            ModelFile modelFile;
                            modelFile.filename = filename;
                            modelFile.quant = detectQuantization(filename);
                            modelFiles.push_back(modelFile);
                        }
                    }
                }
            }
            
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            std::cerr << "Raw response (first 500 chars): " << response.data.substr(0, 500) << std::endl;
            return modelFiles;
        }
        
        // Sort by quantization priority (8-bit first)
        std::sort(modelFiles.begin(), modelFiles.end(), [](const ModelFile& a, const ModelFile& b) {
            return a.quant.priority < b.quant.priority;
        });
        
        return modelFiles;
    }
};

class InteractiveList {
private:
    std::vector<std::string> items;
    std::vector<std::string> filteredItems;
    std::string searchQuery;
    size_t selectedIndex;
    size_t viewportTop;
    size_t maxDisplayItems;
    bool isSearchMode;
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

public:
    InteractiveList(const std::vector<std::string>& listItems) 
        : items(listItems), filteredItems(listItems), searchQuery(""), selectedIndex(0), 
          viewportTop(0), maxDisplayItems(20), isSearchMode(false) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        
        // Calculate max display items based on console height
        // Reserve space for title (4 lines) + search bar (2 lines) + selected info (2 lines) + margin (2 lines)
        int availableHeight = csbi.srWindow.Bottom - csbi.srWindow.Top - 10;
        if (availableHeight > 5) {
            maxDisplayItems = static_cast<size_t>(availableHeight);
        }
    }

    void hideCursor() {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }

    void showCursor() {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = true;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }

    void clearScreen() {
        system("cls");
    }

    void moveCursor(int x, int y) {
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(hConsole, coord);
    }

    void setColor(int color) {
        SetConsoleTextAttribute(hConsole, color);
    }

    void resetColor() {
        SetConsoleTextAttribute(hConsole, csbi.wAttributes);
    }    void updateViewport() {
        // Adjust viewport to keep selected item visible
        if (selectedIndex < viewportTop) {
            viewportTop = selectedIndex;
        } else if (selectedIndex >= viewportTop + maxDisplayItems) {
            viewportTop = selectedIndex - maxDisplayItems + 1;
        }
        
        // Ensure viewport doesn't go beyond bounds
        if (viewportTop + maxDisplayItems > filteredItems.size()) {
            if (filteredItems.size() >= maxDisplayItems) {
                viewportTop = filteredItems.size() - maxDisplayItems;
            } else {
                viewportTop = 0;
            }
        }
    }

    void applyFilter() {
        filteredItems.clear();
        selectedIndex = 0;
        viewportTop = 0;
        
        if (searchQuery.empty()) {
            filteredItems = items;
        } else {
            std::string lowerQuery = searchQuery;
            std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
            
            for (const auto& item : items) {
                std::string lowerItem = item;
                std::transform(lowerItem.begin(), lowerItem.end(), lowerItem.begin(), ::tolower);
                
                if (lowerItem.find(lowerQuery) != std::string::npos) {
                    filteredItems.push_back(item);
                }
            }
        }
    }    void displayList() {
        clearScreen();
        
        // Display title
        std::cout << "Kolosal CLI - Select Model\n";
        std::cout << "Use UP/DOWN arrows to navigate, ENTER to select, ESC or Ctrl+C to exit\n";
        std::cout << "Press '/' to search, BACKSPACE to clear search\n\n";

        // Display search bar
        if (isSearchMode) {
            setColor(BACKGROUND_GREEN | FOREGROUND_INTENSITY);
            std::cout << "Search: " << searchQuery << "_" << std::endl;
            resetColor();
        } else {
            std::cout << "Search: " << searchQuery;
            if (!searchQuery.empty()) {
                setColor(FOREGROUND_INTENSITY);
                std::cout << " (Press '/' to edit)";
                resetColor();
            } else {
                setColor(FOREGROUND_INTENSITY);
                std::cout << " (Press '/' to search)";
                resetColor();
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

        if (filteredItems.empty()) {
            if (searchQuery.empty()) {
                std::cout << "No models available or failed to fetch models.\n";
                std::cout << "Please check your internet connection and try again.\n";
            } else {
                std::cout << "No models found matching: \"" << searchQuery << "\"\n";
                std::cout << "Try a different search term or press BACKSPACE to clear.\n";
            }
            return;
        }

        updateViewport();

        // Calculate display range
        size_t displayStart = viewportTop;
        size_t displayEnd = (viewportTop + maxDisplayItems < filteredItems.size()) ? 
                           viewportTop + maxDisplayItems : filteredItems.size();

        // Show scroll indicator at top if there are items above
        if (viewportTop > 0) {
            setColor(FOREGROUND_INTENSITY);
            std::cout << "  ▲ (" << viewportTop << " more above)\n";
            resetColor();
        }        // Display visible items
        for (size_t i = displayStart; i < displayEnd; ++i) {
            std::string item = filteredItems[i];
            std::string filename = item;
            std::string quantDescription = "";
            
            // Parse quantization info if present (format: "filename (TYPE: description)")
            size_t parenPos = item.find(" (");
            if (parenPos != std::string::npos && item.back() == ')') {
                filename = item.substr(0, parenPos);
                size_t colonPos = item.find(": ", parenPos);
                if (colonPos != std::string::npos) {
                    quantDescription = item.substr(colonPos + 2);
                    quantDescription = quantDescription.substr(0, quantDescription.length() - 1); // Remove closing paren
                }
            }
            
            if (i == selectedIndex) {
                // Highlight selected item
                setColor(BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                std::cout << "> ";
                
                // Print filename with fixed width for alignment
                std::cout << std::left << std::setw(50) << filename;
                resetColor();
                
                // Add quantization description with softer color if present
                if (!quantDescription.empty()) {
                    setColor(FOREGROUND_INTENSITY); // Softer gray color
                    std::cout << " " << quantDescription;
                    resetColor();
                }
                std::cout << std::endl;
            } else {
                std::cout << "  ";
                
                // Print filename with fixed width for alignment
                std::cout << std::left << std::setw(50) << filename;
                
                // Add quantization description with softer color if present
                if (!quantDescription.empty()) {
                    setColor(FOREGROUND_INTENSITY); // Softer gray color
                    std::cout << " " << quantDescription;
                    resetColor();
                }
                std::cout << std::endl;
            }
        }

        // Show scroll indicator at bottom if there are items below
        if (displayEnd < filteredItems.size()) {
            setColor(FOREGROUND_INTENSITY);
            std::cout << "  ▼ (" << (filteredItems.size() - displayEnd) << " more below)\n";
            resetColor();
        }        // Display current selection info
        if (!filteredItems.empty()) {
            std::string selectedItem = filteredItems[selectedIndex];
            std::string selectedFilename = selectedItem;
            
            // Extract just the filename for display
            size_t parenPos = selectedItem.find(" (");
            if (parenPos != std::string::npos) {
                selectedFilename = selectedItem.substr(0, parenPos);
            }
            
            std::cout << "\nSelected: " << selectedFilename;
            std::cout << " (" << (selectedIndex + 1) << "/" << filteredItems.size() << ")";
            if (!searchQuery.empty()) {
                std::cout << " | Filtered from " << items.size() << " total";
            }
            std::cout << std::endl;
        }
    }    int run() {
        hideCursor();
        
        // Handle empty list case
        if (items.empty()) {
            displayList();
            std::cout << "\nPress any key to exit...";
            _getch();
            showCursor();
            return -1;
        }
        
        while (true) {
            displayList();
            
            int key = _getch();
              // Handle special keys (arrow keys)
            if (key == 224) { // Special key prefix
                key = _getch();
                switch (key) {
                    case 72: // Up arrow
                        if (isSearchMode) {
                            // Do nothing when in search mode and pressing up
                        } else if (!filteredItems.empty()) {
                            if (selectedIndex > 0) {
                                selectedIndex--;
                            } else {
                                // When at top of list, go to search input
                                isSearchMode = true;
                            }
                        } else {
                            // If no items, go to search input
                            isSearchMode = true;
                        }
                        break;
                    case 80: // Down arrow
                        if (isSearchMode) {
                            // Exit search mode and go to first item
                            isSearchMode = false;
                            if (!filteredItems.empty()) {
                                selectedIndex = 0;
                            }
                        } else if (!filteredItems.empty()) {
                            if (selectedIndex < filteredItems.size() - 1) {
                                selectedIndex++;
                            } else {
                                selectedIndex = 0; // Wrap to top
                            }
                        }
                        break;
                }
            } else {
                switch (key) {
                    case 13: // Enter
                        if (isSearchMode) {
                            // Exit search mode
                            isSearchMode = false;
                        } else if (!filteredItems.empty()) {
                            // Select item
                            showCursor();
                            clearScreen();
                            std::cout << "You selected: " << filteredItems[selectedIndex] << std::endl;
                            // Find the original index in the items array
                            auto it = std::find(items.begin(), items.end(), filteredItems[selectedIndex]);
                            if (it != items.end()) {
                                return static_cast<int>(std::distance(items.begin(), it));
                            }
                            return -1;
                        }
                        break;
                    case 27: // Escape
                        if (isSearchMode) {
                            // Exit search mode
                            isSearchMode = false;
                        } else {
                            showCursor();
                            clearScreen();
                            std::cout << "Operation cancelled." << std::endl;
                            return -1;
                        }
                        break;
                    case 3: // Ctrl+C
                        showCursor();
                        clearScreen();
                        std::cout << "Interrupted by user." << std::endl;
                        return -1;
                    case 8: // Backspace
                        if (isSearchMode && !searchQuery.empty()) {
                            searchQuery.pop_back();
                            applyFilter();
                        } else if (!isSearchMode && !searchQuery.empty()) {
                            // Clear search when not in search mode
                            searchQuery.clear();
                            applyFilter();
                        }
                        break;
                    case '/': // Enter search mode
                        isSearchMode = true;
                        break;
                    default:
                        if (isSearchMode && key >= 32 && key <= 126) { // Printable characters
                            searchQuery += static_cast<char>(key);
                            applyFilter();
                        }
                        break;
                }
            }
        }    }
};

int main() {
    // Initialize curl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    std::cout << "Welcome to Kolosal CLI!\n";
    std::cout << "Fetching available models from Hugging Face...\n\n";
    
    // Fetch models from Hugging Face API
    std::vector<std::string> models = HuggingFaceClient::fetchKolosalModels();
    
    if (models.empty()) {
        std::cout << "No Kolosal models found or failed to fetch models.\n";
        std::cout << "This could be due to:\n";
        std::cout << "1. No internet connection\n";
        std::cout << "2. No public models in the kolosal organization\n";
        std::cout << "3. API request failed\n\n";
        std::cout << "Showing sample menu instead...\n\n";
        
        // Fallback to sample menu
        models = {
            "kolosal/sample-model-1",
            "kolosal/sample-model-2", 
            "kolosal/sample-model-3",
            "Back to Main Menu"
        };
        
        Sleep(2000);
    } else {
        std::cout << "Found " << models.size() << " Kolosal models!\n\n";
        // Add navigation option
        models.push_back("Back to Main Menu");
        Sleep(1000);
    }    InteractiveList menu(models);
    int result = menu.run();

    if (result >= 0 && result < static_cast<int>(models.size()) - 1) {
        std::string selectedModel = models[result];
        std::cout << "You selected model: " << selectedModel << std::endl;
          // Fetch .gguf files for the selected model
        std::cout << "\nFetching available .gguf files...\n\n";
        std::vector<ModelFile> modelFiles = HuggingFaceClient::fetchModelFilesWithQuant(selectedModel);
        
        std::vector<std::string> displayFiles;
        
        if (modelFiles.empty()) {
            std::cout << "No .gguf files found for this model.\n";
            std::cout << "This could be due to:\n";
            std::cout << "1. Model doesn't have quantized .gguf files\n";
            std::cout << "2. Files are in a different format\n";
            std::cout << "3. API request failed\n\n";
            std::cout << "Showing sample files instead...\n\n";
              // Fallback to sample files with quantization info
            std::string modelName = selectedModel.substr(selectedModel.find('/') + 1);
            
            ModelFile file1;
            file1.filename = modelName + "-Q8_0.gguf";
            file1.quant = detectQuantization(file1.filename);
            modelFiles.push_back(file1);
            
            ModelFile file2;
            file2.filename = modelName + "-Q4_K_M.gguf";
            file2.quant = detectQuantization(file2.filename);
            modelFiles.push_back(file2);
            
            ModelFile file3;
            file3.filename = modelName + "-Q5_K_M.gguf";
            file3.quant = detectQuantization(file3.filename);
            modelFiles.push_back(file3);
            
            Sleep(2000);
        } else {
            std::cout << "Found " << modelFiles.size() << " .gguf files!\n\n";
            Sleep(1000);
        }
        
        // Convert ModelFile objects to display strings
        for (const auto& file : modelFiles) {
            displayFiles.push_back(file.getDisplayName());
        }
        displayFiles.push_back("Back to Model Selection");
        
        // Show .gguf files in interactive list with default selection on Q8_0 (8-bit)
        InteractiveList fileMenu(displayFiles);
        
        // Find and set default selection to Q8_0 (highest priority 8-bit quantization)
        for (size_t i = 0; i < modelFiles.size(); ++i) {
            if (modelFiles[i].quant.type == "Q8_0") {
                // The InteractiveList constructor already sets selectedIndex to 0, but we can note this for future enhancement
                break;
            }
        }
        
        int fileResult = fileMenu.run();
        
        if (fileResult >= 0 && fileResult < static_cast<int>(modelFiles.size())) {
            const ModelFile& selectedFile = modelFiles[fileResult];
            std::cout << "You selected file: " << selectedFile.filename << std::endl;
            std::cout << "Quantization: " << selectedFile.quant.type << " - " << selectedFile.quant.description << std::endl;
            std::cout << "From model: " << selectedModel << std::endl;
            std::cout << "Download URL: https://huggingface.co/" << selectedModel << "/resolve/main/" << selectedFile.filename << std::endl;            std::cout << "\nFile download feature coming soon!" << std::endl;
        } else if (fileResult == static_cast<int>(displayFiles.size()) - 1) {
            std::cout << "Returning to model selection..." << std::endl;
        } else {
            std::cout << "File selection cancelled." << std::endl;
        }
        
    } else if (result == static_cast<int>(models.size()) - 1) {
        std::cout << "Returning to main menu..." << std::endl;
    } else {
        std::cout << "Model selection cancelled." << std::endl;
    }

    std::cout << "\nPress any key to exit...";
    _getch();
    
    // Cleanup curl
    curl_global_cleanup();
    
    return 0;
}