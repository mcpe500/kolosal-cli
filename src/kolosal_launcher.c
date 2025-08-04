#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

int main(int argc, char *argv[]) {
    // Get the path to the executable
    char *exe_path = argv[0];
    char *exe_dir = dirname(strdup(exe_path));
    
    // Build path to the shell script
    char script_path[1024];
    snprintf(script_path, sizeof(script_path), "%s/kolosal-launcher.sh", exe_dir);
    
    // Check if the script exists
    struct stat st;
    if (stat(script_path, &st) != 0) {
        fprintf(stderr, "Error: kolosal-launcher.sh not found at %s\n", script_path);
        return 1;
    }
    
    // Execute the shell script
    execl("/bin/bash", "bash", script_path, (char *)NULL);
    
    // If we get here, execl failed
    perror("Failed to execute kolosal-launcher.sh");
    return 1;
}
