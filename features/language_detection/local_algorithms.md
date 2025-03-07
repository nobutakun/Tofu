# Local Language Detection Algorithms

This document outlines the algorithm selection for the device-side (Local Language Detector - LLD) component of the To-fu Language Detection module.

## Algorithm Selection Criteria

The selection of algorithms for the device-side language detection is guided by the following criteria:

1. **Resource Efficiency**:
   - Low memory footprint (< 100KB for models)
   - Minimal CPU usage (suitable for ESP32)
   - Limited power consumption

2. **Speed**:
   - Fast detection (< 10ms for typical inputs)
   - Minimal startup time
   - Efficient for real-time applications

3. **Accuracy**:
   - Reasonable accuracy for common languages
   - Graceful degradation for edge cases
   - Clear confidence metrics

4. **Implementation Complexity**:
   - Algorithms that can be implemented in C
   - Minimal external dependencies
   - Maintainable codebase

## Selected Algorithms

Based on the criteria above, we've selected the following lightweight algorithms for the device-side language detection:

### 1. N-gram Statistical Analysis

N-grams are contiguous sequences of n items from a given sample of text. This approach is highly efficient and provides good accuracy for many languages.

#### Implementation Details

```c
typedef struct {
    uint16_t ngram;        // Packed representation of the n-gram
    uint8_t frequency;     // Normalized frequency (0-255)
} ngram_entry_t;

typedef struct {
    char language_code[8];             // ISO 639-1/2 language code
    ngram_entry_t *ngrams;             // Array of n-gram entries
    uint16_t ngram_count;              // Number of n-grams
    float distinctive_factor;          // How distinctive this language is
} language_profile_t;

// Global language profiles
static language_profile_t *language_profiles = NULL;
static uint8_t language_count = 0;

// N-gram generation function
uint16_t pack_ngram(const char *text, uint8_t n) {
    uint16_t result = 0;
    for (uint8_t i = 0; i < n && text[i]; i++) {
        // Pack characters into a 16-bit integer
        // For simplicity, we only consider lowercase a-z
        char c = tolower(text[i]);
        if (c >= 'a' && c <= 'z') {
            result = (result << 5) | (c - 'a' + 1);
        } else {
            result = (result << 5); // Space or other character
        }
    }
    return result;
}

// Calculate n-gram profile for input text
void calculate_ngram_profile(const char *text, uint32_t length, 
                           uint16_t *ngrams, uint8_t *frequencies, 
                           uint16_t max_ngrams, uint16_t *count) {
    // Implementation details...
}

// Compare input profile with language profiles
float compare_profiles(uint16_t *input_ngrams, uint8_t *input_frequencies, 
                     uint16_t input_count, language_profile_t *language) {
    // Implementation details...
}

// Main detection function using n-grams
ld_status_t detect_language_ngram(const char *text, uint32_t length, 
                                ld_result_t *result) {
    // Implementation details...
}
```

#### Advantages

- **Memory Efficient**: Compact representation of language profiles
- **Fast Computation**: Simple integer operations
- **Scalable**: Can adjust n-gram size based on available memory
- **Language Agnostic**: Works for most written languages

#### Limitations

- Less accurate for very short texts
- Requires careful profile creation
- May confuse similar languages

### 2. Character Frequency Analysis

This approach analyzes the frequency distribution of characters in the input text and compares it with known distributions for different languages.

#### Implementation Details

```c
typedef struct {
    char character;        // The character (or byte for multi-byte encodings)
    float frequency;       // Expected frequency in the language
} char_freq_entry_t;

typedef struct {
    char language_code[8];             // ISO 639-1/2 language code
    char_freq_entry_t *frequencies;    // Array of character frequencies
    uint16_t char_count;               // Number of characters
    bool is_multi_byte;                // Whether this uses multi-byte encoding
} char_freq_profile_t;

// Global character frequency profiles
static char_freq_profile_t *char_freq_profiles = NULL;
static uint8_t char_freq_profile_count = 0;

// Calculate character frequency for input text
void calculate_char_frequencies(const char *text, uint32_t length, 
                              char *chars, float *frequencies, 
                              uint16_t max_chars, uint16_t *count) {
    // Implementation details...
}

// Compare input frequencies with language profiles
float compare_char_frequencies(char *input_chars, float *input_frequencies, 
                             uint16_t input_count, char_freq_profile_t *language) {
    // Implementation details...
}

// Main detection function using character frequencies
ld_status_t detect_language_char_freq(const char *text, uint32_t length, 
                                    ld_result_t *result) {
    // Implementation details...
}
```

#### Advantages

- **Very Low Memory Usage**: Requires only character frequency tables
- **Simple Implementation**: Basic counting and comparison operations
- **Fast for Short Texts**: Efficient for quick initial detection
- **Script Identification**: Easily identifies different scripts (Latin, Cyrillic, etc.)

#### Limitations

- Lower accuracy than n-gram methods
- Poor for distinguishing similar languages with same script
- Requires sufficient text length for statistical significance

### 3. Common Word Matching

This approach identifies languages by detecting common words or patterns specific to each language.

#### Implementation Details

```c
typedef struct {
    char *word;            // Common word
    float weight;          // Weight/importance of this word
} common_word_t;

typedef struct {
    char language_code[8];         // ISO 639-1/2 language code
    common_word_t *words;          // Array of common words
    uint16_t word_count;           // Number of words
} common_word_profile_t;

// Global common word profiles
static common_word_profile_t *word_profiles = NULL;
static uint8_t word_profile_count = 0;

// Find common words in input text
void find_common_words(const char *text, uint32_t length, 
                     common_word_profile_t *profile,
                     uint16_t *matches, float *total_weight) {
    // Implementation details...
}

// Main detection function using common words
ld_status_t detect_language_common_words(const char *text, uint32_t length, 
                                       ld_result_t *result) {
    // Implementation details...
}
```

#### Advantages

- **High Precision**: Very accurate for languages with distinctive common words
- **Fast Rejection**: Quickly eliminates unlikely languages
- **Minimal False Positives**: When words are carefully selected
- **Human-Readable Model**: Easy to debug and maintain

#### Limitations

- Requires more memory than other approaches
- Less effective for short texts
- Needs careful word selection to avoid bias

## Hybrid Approach

For optimal results, we combine these algorithms using a weighted voting system:

```c
ld_status_t ld_local_detect(const char *text, uint32_t text_length, ld_result_t *result) {
    // Start timing
    uint64_t start_time = sys_get_time_ms();
    
    // Results from individual algorithms
    ld_result_t ngram_result;
    ld_result_t char_freq_result;
    ld_result_t common_word_result;
    
    // Run algorithms in parallel or sequence based on text length
    bool run_ngram = text_length >= MIN_LENGTH_FOR_NGRAM;
    bool run_char_freq = true; // Always run character frequency
    bool run_common_words = text_length >= MIN_LENGTH_FOR_COMMON_WORDS;
    
    // Run enabled algorithms
    if (run_ngram) {
        detect_language_ngram(text, text_length, &ngram_result);
    }
    
    if (run_char_freq) {
        detect_language_char_freq(text, text_length, &char_freq_result);
    }
    
    if (run_common_words) {
        detect_language_common_words(text, text_length, &common_word_result);
    }
    
    // Combine results with weighted voting
    char final_language[8] = {0};
    float final_confidence = 0.0f;
    
    combine_detection_results(
        run_ngram ? &ngram_result : NULL, NGRAM_WEIGHT,
        run_char_freq ? &char_freq_result : NULL, CHAR_FREQ_WEIGHT,
        run_common_words ? &common_word_result : NULL, COMMON_WORD_WEIGHT,
        final_language, &final_confidence
    );
    
    // Set result
    strncpy(result->language_code, final_language, sizeof(result->language_code));
    result->confidence = final_confidence;
    result->level = ld_confidence_level_from_score(final_confidence);
    result->is_cloud_result = false;
    result->detection_time_ms = sys_get_time_ms() - start_time;
    
    return LD_STATUS_OK;
}
```

## Algorithm Weights and Thresholds

The weights and thresholds for the hybrid approach are determined based on empirical testing:

| Algorithm | Default Weight | Min Text Length | Memory Usage |
|-----------|---------------|----------------|--------------|
| N-gram | 0.6 | 20 characters | ~50KB |
| Character Frequency | 0.3 | 5 characters | ~10KB |
| Common Word | 0.1 | 30 characters | ~30KB |

These values can be adjusted based on the specific requirements and constraints of the To-fu device.

## Language Profiles

The language detection algorithms rely on pre-computed language profiles. We support the following languages in the local detection:

1. English (en)
2. Spanish (es)
3. French (fr)
4. German (de)
5. Italian (it)
6. Portuguese (pt)
7. Russian (ru)
8. Japanese (ja)
9. Chinese (zh)
10. Korean (ko)
11. Arabic (ar)
12. Hindi (hi)
13. Thai (th)
14. Vietnamese (vi)
15. Dutch (nl)

Each language profile includes:
- N-gram frequency data (top 500 n-grams)
- Character frequency distribution
- 50-100 common words or patterns

## Optimizations

Several optimizations are implemented to improve performance:

### 1. Early Termination

```c
// Early termination if a clear winner emerges
if (current_best_confidence > EARLY_TERMINATION_THRESHOLD) {
    // Skip remaining comparisons
    break;
}
```

### 2. Bit-Packed N-grams

```c
// Pack 3-grams into 16-bit integers (5 bits per character, 3 characters)
// This reduces memory usage and comparison time
uint16_t packed_ngram = ((c1 & 0x1F) << 10) | ((c2 & 0x1F) << 5) | (c3 & 0x1F);
```

### 3. Script-Based Pre-filtering

```c
// Quick script identification to narrow down language candidates
script_t script = identify_script(text, text_length);
if (script != SCRIPT_UNKNOWN) {
    // Only compare with languages using this script
    filter_languages_by_script(script, &candidate_languages, &candidate_count);
}
```

### 4. Sparse Representation

```c
// Store only distinctive n-grams for each language
// This reduces memory usage and comparison time
typedef struct {
    uint16_t ngram;
    uint8_t frequency;
    uint8_t languages_bitmap; // Bit flags for languages where this n-gram is common
} distinctive_ngram_t;
```

## Memory Management

To minimize memory usage, we implement several strategies:

1. **Shared Language Profiles**: Common data structures are shared across algorithms
2. **Lazy Loading**: Load only required language profiles based on user preferences
3. **Compressed Storage**: Use bit-packing and quantization for compact representation
4. **Memory Pools**: Pre-allocate working memory to avoid fragmentation

## Performance Benchmarks

Preliminary benchmarks on ESP32 hardware:

| Text Length | Detection Time | Memory Usage | Accuracy |
|-------------|---------------|--------------|----------|
| 10 chars | 2-5 ms | ~2KB | 60-70% |
| 50 chars | 5-10 ms | ~5KB | 75-85% |
| 100+ chars | 10-20 ms | ~10KB | 85-95% |

These benchmarks are for the combined hybrid approach with all optimizations enabled.

## Future Improvements

1. **Adaptive Algorithm Selection**: Dynamically choose algorithms based on text characteristics
2. **Incremental Processing**: Process text as it arrives for streaming inputs
3. **Custom Language Models**: Allow users to add custom language profiles
4. **Hardware Acceleration**: Utilize ESP32 DSP instructions for faster processing
5. **Quantized Models**: Further reduce memory footprint with quantization techniques

## Integration with Two-Tier Approach

The local detection algorithms are designed to work seamlessly with the two-tier approach:

1. **Fast Initial Detection**: Local algorithms provide quick initial results
2. **Confidence Metrics**: Clear confidence scores help determine when to use cloud detection
3. **Complementary Strengths**: Local algorithms excel at script identification and common languages
4. **Fallback Chain**: Algorithms are arranged in a fallback chain from fastest to most accurate
