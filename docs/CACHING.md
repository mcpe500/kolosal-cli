# Caching System

## Overview

The Kolosal CLI now includes a smart caching mechanism to significantly reduce API calls to Hugging Face and improve application performance. The cache system stores both model lists and model file information locally.

## Features

### Two-Level Caching
- **Memory Cache**: Fast in-memory storage for the current session
- **Disk Cache**: Persistent storage across application restarts

### Automatic Cache Management
- **TTL (Time To Live)**: Cached data expires automatically
  - Models list: 1 hour TTL
  - Model files: 30 minutes TTL
- **Cache Validation**: Automatically checks if cached data is still valid
- **Fallback**: Seamlessly falls back to API requests when cache misses occur

### Cache Storage
- **Windows**: `%APPDATA%\kolosal-cli\cache\`
- **Unix/Linux**: `~/.cache/kolosal-cli/`
- **Fallback**: `./cache/` in current directory if system cache fails

## Benefits

### Performance Improvements
- **Faster Startup**: Cached model lists load instantly
- **Reduced Network Traffic**: Fewer API calls to Hugging Face
- **Better User Experience**: No waiting for repeated API requests
- **Offline Capability**: Recently cached data available without internet

### Resource Efficiency
- **Bandwidth Savings**: Significantly reduces data transfer
- **API Rate Limiting**: Helps avoid hitting Hugging Face API limits
- **Server Load**: Reduces load on Hugging Face servers

## Cache Management

### Interactive Cache Menu
The application now includes a cache management menu with options to:

1. **Continue to Model Selection**: Proceed with the main application
2. **Clear Cache**: Remove all cached data
3. **View Cache Status**: Display cache configuration and status
4. **Exit Application**: Quit the application

### Manual Cache Operations
Users can clear the cache when needed, which is useful for:
- Forcing fresh data retrieval
- Troubleshooting cache-related issues
- Freeing up disk space

## Technical Implementation

### Cache Structure
```
cache/
├── kolosal_models.cache          # Cached models list
├── model_files_kolosal_model1.cache  # Files for model1
├── model_files_kolosal_model2.cache  # Files for model2
└── ...
```

### Cache File Format
- **Format**: JSON with metadata
- **Content**: Original API response data
- **Metadata**: Timestamp for TTL validation

### Thread Safety
- Cache operations are designed to be safe for single-threaded usage
- File-based cache prevents corruption from multiple instances

## Behavior Changes

### First Run
- Cache is empty, so API calls are made as normal
- Results are automatically cached for future use

### Subsequent Runs
- Cache is checked first for valid data
- Fresh API calls only made when cache expires or misses
- User sees "Cache hit" messages when using cached data

### Network Issues
- If API calls fail, application attempts to use any available cached data
- Graceful degradation to sample data if both cache and API fail

## Configuration

The cache system is automatically configured with sensible defaults:
- **Models TTL**: 1 hour (3600 seconds)
- **Model Files TTL**: 30 minutes (1800 seconds)
- **Cache Location**: System-appropriate cache directory

These values can be modified in the `CacheManager` class if needed.

## Troubleshooting

### Cache Issues
1. **Use "Clear Cache" option** from the cache menu
2. **Check disk space** in the cache directory
3. **Verify permissions** for cache directory access

### Performance Issues
1. **Cache hits** should show performance improvements
2. **Cache misses** will perform normal API requests
3. **Monitor console output** for cache status messages

## Future Enhancements

Potential improvements for the caching system:
- Configurable TTL values
- Cache size limits and cleanup
- Background cache refresh
- Cache statistics and analytics
- Compression for larger cache files
