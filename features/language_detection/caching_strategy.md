# Language Detection Caching Strategy

This document outlines the caching strategy for the Language Detection (LD) module to optimize performance and reduce resource usage for frequently detected language patterns.

## Caching Architecture

The Language Detection module implements a multi-level caching strategy to optimize performance:

```
┌─────────────────────────────────────────────────────────────┐
│                      To-fu Device                           │
│                                                             │
│  ┌─────────────────┐        ┌───────────────────────────┐   │
│  │                 │        │                           │   │
│  │  Input Text     │───────►│  L1: Exact Match Cache    │   │
│  │                 │        │                           │   │
│  └─────────────────┘        └───────────┬───────────────┘   │
│                                         │                   │
│                                         │ Cache Miss        │
│                                         ▼                   │
│                             ┌───────────────────────────┐   │
│                             │                           │   │
│                             │  L2: Pattern Cache        │   │
│                             │                           │   │
│                             └───────────┬───────────────┘   │
│                                         │                   │
│                                         │ Cache Miss        │
│                                         ▼                   │
│                             ┌───────────────────────────┐   │
│                             │                           │   │
│                             │  Language Detection       │   │
│                             │  (Local or Cloud)         │   │
│                             │                           │   │
│                             └───────────┬───────────────┘   │
│                                         │                   │
│                                         │                   │
│                                         ▼                   │
│                             ┌───────────────────────────┐   │
│                             │                           │   │
│                             │  Cache Update Logic       │   │
│                             │                           │   │
│                             └───────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Cache Levels

### L1: Exact Match Cache

The first level of caching stores exact matches of previously detected text:

1. **Structure**:
   - Hash table with text as key and detection result as value
   - Fixed-size cache with LRU (Least Recently Used) eviction policy
   - Each entry includes timestamp for TTL (Time-To-Live) management

2. **Lookup Process**:
   - Calculate hash of input text
   - Check for exact match in hash table
   - Verify entry hasn't expired (based on TTL)
   - Return cached result if valid match found

3. **Performance Characteristics**:
   - O(1) lookup time
   - Very high accuracy (100% for exact matches)
   - Optimal for repeated exact phrases

### L2: Pattern Cache

The second level caches language patterns rather than exact text:

1. **Structure**:
   - N-gram frequency tables for different languages
   - Character distribution patterns
   - Common word dictionaries for supported languages
   - Language-specific feature vectors

2. **Lookup Process**:
   - Extract features from input text (n-grams, character distribution)
   - Compare against cached patterns
   - Calculate similarity scores for each language
   - Return result if confidence exceeds threshold

3. **Performance Characteristics**:
   - O(n) lookup time (where n is text length)
   - High accuracy for similar text patterns
   - Effective for variations of similar content

## Cache Management

### Memory Allocation

The cache memory is allocated based on device capabilities and configuration:

1. **Default Allocation**:
   - L1 Cache: 64KB (configurable)
   - L2 Cache: 128KB (configurable)

2. **Dynamic Sizing**:
   - Cache size can be adjusted based on available memory
   - Minimum and maximum bounds are enforced
   - Automatic resizing based on hit/miss ratio

3. **Entry Size Management**:
   - Variable-length text storage with maximum limit
   - Fixed-size result structures
   - Metadata overhead minimized

### Eviction Policies

Multiple eviction policies are implemented to optimize cache performance:

1. **LRU (Least Recently Used)**:
   - Primary eviction policy for L1 cache
   - Tracks access timestamps for each entry
   - Evicts least recently accessed entries when cache is full

2. **Frequency-Based**:
   - Secondary policy that considers access frequency
   - Protects frequently accessed entries from eviction
   - Implemented using a counter for each entry

3. **TTL (Time-To-Live)**:
   - All cache entries have an expiration time
   - Default TTL: 24 hours (configurable)
   - Different TTL values for different confidence levels
   - Automatic cleanup of expired entries

4. **Confidence-Based**:
   - Entries with higher confidence scores have longer TTL
   - Low-confidence results expire faster
   - Prevents persistence of potentially incorrect results

### Cache Coherence

Mechanisms to ensure cache validity:

1. **Invalidation Triggers**:
   - Language model updates
   - User-corrected detection results
   - Configuration changes
   - Device locale changes

2. **Partial Invalidation**:
   - Selective invalidation based on language
   - Confidence threshold-based invalidation
   - Age-based invalidation

## Implementation Details

### Data Structures

```c
// L1 Cache Entry
typedef struct {
    char *text;                 // Original text
    uint32_t text_length;       // Text length
    uint32_t hash;              // Text hash for quick comparison
    ld_result_t result;         // Detection result
    uint64_t timestamp;         // Last access time
    uint32_t access_count;      // Number of times accessed
} ld_cache_entry_t;

// L2 Cache Pattern
typedef struct {
    uint32_t ngram_counts[MAX_LANGUAGES][MAX_NGRAMS]; // N-gram frequency table
    float char_distribution[MAX_LANGUAGES][CHAR_SET_SIZE]; // Character distribution
    float feature_vectors[MAX_LANGUAGES][FEATURE_VECTOR_SIZE]; // Feature vectors
    uint64_t timestamp;         // Last update time
    uint32_t access_count;      // Number of times accessed
} ld_pattern_cache_t;

// Cache Manager
typedef struct {
    // L1 Cache
    ld_cache_entry_t *l1_entries;
    uint32_t l1_capacity;
    uint32_t l1_count;
    uint32_t l1_hits;
    uint32_t l1_misses;
    
    // L2 Cache
    ld_pattern_cache_t l2_patterns;
    uint32_t l2_hits;
    uint32_t l2_misses;
    
    // Configuration
    uint32_t default_ttl_ms;
    uint32_t max_text_length;
    float min_confidence_for_cache;
} ld_cache_manager_t;
```

### Cache Operations

#### Lookup Algorithm

```c
ld_status_t ld_cache_lookup(const char *text, uint32_t text_length, ld_result_t *result, bool *found) {
    // Initialize output parameters
    *found = false;
    
    // Check if caching is enabled
    if (!ld_state.config.enable_caching) {
        return LD_STATUS_OK;
    }
    
    // Get current time
    uint64_t current_time = sys_get_time_ms();
    
    // Calculate text hash
    uint32_t hash = calculate_hash(text, text_length);
    
    // L1 Cache: Check for exact match
    for (uint32_t i = 0; i < ld_state.cache.l1_count; i++) {
        ld_cache_entry_t *entry = &ld_state.cache.l1_entries[i];
        
        // Quick hash comparison first
        if (entry->hash == hash && 
            entry->text_length == text_length &&
            memcmp(entry->text, text, text_length) == 0) {
            
            // Check if entry has expired
            if (current_time - entry->timestamp > ld_state.cache.default_ttl_ms) {
                // Entry expired, remove it
                free(entry->text);
                
                // Move last entry to this position if not already the last
                if (i < ld_state.cache.l1_count - 1) {
                    memcpy(entry, &ld_state.cache.l1_entries[ld_state.cache.l1_count - 1], sizeof(ld_cache_entry_t));
                    memset(&ld_state.cache.l1_entries[ld_state.cache.l1_count - 1], 0, sizeof(ld_cache_entry_t));
                } else {
                    memset(entry, 0, sizeof(ld_cache_entry_t));
                }
                
                ld_state.cache.l1_count--;
                continue;
            }
            
            // Found valid cache entry
            *found = true;
            memcpy(result, &entry->result, sizeof(ld_result_t));
            
            // Update access statistics
            entry->timestamp = current_time;
            entry->access_count++;
            ld_state.cache.l1_hits++;
            
            return LD_STATUS_OK;
        }
    }
    
    // L1 Cache miss
    ld_state.cache.l1_misses++;
    
    // L2 Cache: Check for pattern match
    if (text_length >= MIN_TEXT_LENGTH_FOR_PATTERN_MATCH) {
        float confidence = 0.0f;
        char language_code[8] = {0};
        
        if (check_pattern_match(text, text_length, language_code, &confidence)) {
            // Pattern match found with sufficient confidence
            if (confidence >= ld_state.config.min_confidence_for_cache) {
                *found = true;
                
                // Fill result
                strncpy(result->language_code, language_code, sizeof(result->language_code));
                result->confidence = confidence;
                result->level = ld_confidence_level_from_score(confidence);
                result->is_cloud_result = false;
                result->detection_time_ms = 0; // Pattern match is instant
                
                // Update statistics
                ld_state.cache.l2_hits++;
                
                return LD_STATUS_OK;
            }
        }
    }
    
    // L2 Cache miss
    ld_state.cache.l2_misses++;
    
    return LD_STATUS_OK;
}
```

#### Update Algorithm

```c
ld_status_t ld_cache_update(const char *text, uint32_t text_length, const ld_result_t *result) {
    // Check if caching is enabled
    if (!ld_state.config.enable_caching) {
        return LD_STATUS_OK;
    }
    
    // Check if result confidence meets minimum threshold for caching
    if (result->confidence < ld_state.config.min_confidence_for_cache) {
        return LD_STATUS_OK;
    }
    
    // Calculate text hash
    uint32_t hash = calculate_hash(text, text_length);
    
    // Check if text is already in cache
    for (uint32_t i = 0; i < ld_state.cache.l1_count; i++) {
        if (ld_state.cache.l1_entries[i].hash == hash && 
            ld_state.cache.l1_entries[i].text_length == text_length &&
            memcmp(ld_state.cache.l1_entries[i].text, text, text_length) == 0) {
            
            // Update existing entry
            memcpy(&ld_state.cache.l1_entries[i].result, result, sizeof(ld_result_t));
            ld_state.cache.l1_entries[i].timestamp = sys_get_time_ms();
            ld_state.cache.l1_entries[i].access_count++;
            
            return LD_STATUS_OK;
        }
    }
    
    // Need to add new entry
    
    // Check if cache is full
    if (ld_state.cache.l1_count >= ld_state.cache.l1_capacity) {
        // Find entry to evict (LRU policy)
        uint32_t lru_index = 0;
        uint64_t oldest_time = UINT64_MAX;
        
        for (uint32_t i = 0; i < ld_state.cache.l1_count; i++) {
            // Consider both timestamp and access frequency
            uint64_t adjusted_time = ld_state.cache.l1_entries[i].timestamp + 
                                    (ld_state.cache.l1_entries[i].access_count * ACCESS_COUNT_WEIGHT);
            
            if (adjusted_time < oldest_time) {
                oldest_time = adjusted_time;
                lru_index = i;
            }
        }
        
        // Free the old entry's text
        if (ld_state.cache.l1_entries[lru_index].text) {
            free(ld_state.cache.l1_entries[lru_index].text);
        }
        
        // Reuse this slot
        ld_cache_entry_t *entry = &ld_state.cache.l1_entries[lru_index];
        
        // Allocate and copy text
        entry->text = (char *)malloc(text_length + 1);
        if (entry->text == NULL) {
            return LD_STATUS_ERROR_MEMORY;
        }
        
        memcpy(entry->text, text, text_length);
        entry->text[text_length] = '\0';
        
        // Set other fields
        entry->text_length = text_length;
        entry->hash = hash;
        memcpy(&entry->result, result, sizeof(ld_result_t));
        entry->timestamp = sys_get_time_ms();
        entry->access_count = 1;
    } else {
        // Add to next available slot
        ld_cache_entry_t *entry = &ld_state.cache.l1_entries[ld_state.cache.l1_count];
        
        // Allocate and copy text
        entry->text = (char *)malloc(text_length + 1);
        if (entry->text == NULL) {
            return LD_STATUS_ERROR_MEMORY;
        }
        
        memcpy(entry->text, text, text_length);
        entry->text[text_length] = '\0';
        
        // Set other fields
        entry->text_length = text_length;
        entry->hash = hash;
        memcpy(&entry->result, result, sizeof(ld_result_t));
        entry->timestamp = sys_get_time_ms();
        entry->access_count = 1;
        
        // Increment count
        ld_state.cache.l1_count++;
    }
    
    // Update L2 pattern cache
    update_pattern_cache(text, text_length, result);
    
    return LD_STATUS_OK;
}
```

### Pattern Matching

The pattern matching algorithm uses n-gram analysis and character distribution:

```c
bool check_pattern_match(const char *text, uint32_t text_length, 
                         char *language_code, float *confidence) {
    // Extract features from text
    float features[FEATURE_VECTOR_SIZE];
    extract_features(text, text_length, features);
    
    // Compare with cached patterns
    float max_similarity = 0.0f;
    int best_language_index = -1;
    
    for (int i = 0; i < MAX_LANGUAGES; i++) {
        float similarity = calculate_similarity(features, 
                                              ld_state.cache.l2_patterns.feature_vectors[i]);
        
        if (similarity > max_similarity) {
            max_similarity = similarity;
            best_language_index = i;
        }
    }
    
    // Check if we found a good match
    if (max_similarity >= PATTERN_MATCH_THRESHOLD && best_language_index >= 0) {
        // Get language code for best match
        get_language_code(best_language_index, language_code);
        *confidence = max_similarity;
        return true;
    }
    
    return false;
}
```

## Performance Optimizations

### Memory Efficiency

1. **Text Storage Optimization**:
   - Store hash values for quick comparison
   - Implement text deduplication for similar entries
   - Use reference counting for shared text segments

2. **Compact Data Structures**:
   - Bit-packed language codes
   - Quantized confidence values
   - Compressed timestamp storage

3. **Memory Pool**:
   - Pre-allocated memory pool for cache entries
   - Reduces fragmentation from frequent allocations/deallocations
   - Configurable pool size based on device capabilities

### Computational Efficiency

1. **Hash Function Selection**:
   - Fast, collision-resistant hash function (MurmurHash3)
   - Optimized for text input
   - Incremental hash calculation for streaming input

2. **Lookup Optimizations**:
   - Early rejection based on text length
   - Hash-based quick rejection
   - Bloom filter pre-check for L1 cache

3. **Pattern Matching Optimizations**:
   - Sparse feature vectors
   - Dimensionality reduction for feature comparison
   - Early termination of similarity calculation

## Cache Monitoring and Tuning

### Performance Metrics

The cache system tracks the following metrics:

1. **Hit Rates**:
   - L1 cache hit rate
   - L2 cache hit rate
   - Combined hit rate

2. **Latency Metrics**:
   - Average lookup time
   - Cache update time
   - End-to-end detection time with/without cache

3. **Memory Usage**:
   - Current cache size
   - Peak cache size
   - Memory overhead percentage

### Adaptive Tuning

The cache system can self-tune based on usage patterns:

1. **TTL Adjustment**:
   - Increase TTL for frequently accessed entries
   - Decrease TTL for languages with frequent updates
   - Adjust based on confidence level

2. **Cache Size Adjustment**:
   - Expand cache size if hit rate is high and memory is available
   - Reduce cache size if hit rate is low or memory pressure is high
   - Balance between L1 and L2 cache sizes

3. **Pattern Weighting**:
   - Adjust feature weights based on detection accuracy
   - Prioritize more discriminative features
   - Adapt to user's language usage patterns

## Integration with System

### Power Management

The cache system integrates with the device power management:

1. **Low Power Mode**:
   - Reduce cache size
   - Increase lookup thresholds
   - Disable pattern cache updates

2. **Battery Optimization**:
   - Batch cache maintenance operations
   - Defer non-critical updates
   - Prioritize cache hits to avoid detection processing

### Persistence

Cache persistence strategies:

1. **Volatile Cache**:
   - L1 cache is entirely in-memory
   - Cleared on device restart

2. **Semi-Persistent Cache**:
   - L2 pattern cache can be persisted to flash storage
   - Loaded on initialization
   - Periodic snapshots to reduce startup time

## Future Enhancements

1. **Distributed Caching**:
   - Share pattern cache across leader-follower devices
   - Synchronize high-confidence detections
   - Collaborative cache warming

2. **Predictive Caching**:
   - Pre-compute likely language detections
   - Context-aware cache prefetching
   - User behavior-based cache prioritization

3. **Neural Network Acceleration**:
   - Hardware-accelerated pattern matching
   - Quantized neural networks for language identification
   - Custom DSP algorithms for feature extraction
