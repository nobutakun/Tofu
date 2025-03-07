import { LanguageDetectionResult } from '../types';
import { Logger } from 'winston';

export class LanguageDetectionFallback {
  constructor(private logger: Logger) {
    this.logger = logger.child({ service: 'LanguageDetectionFallback' });
  }

  // Character ranges for different scripts
  private static readonly SCRIPTS = {
    // Japanese scripts (Hiragana, Katakana, and common Kanji)
    JAPANESE: /[\u3040-\u309F\u30A0-\u30FF\u4E00-\u9FFF]/,
    
    // Korean Hangul
    KOREAN: /[\uAC00-\uD7AF\u1100-\u11FF]/,
    
    // Chinese (Traditional and Simplified)
    CHINESE: /[\u4E00-\u9FFF\u3400-\u4DBF]/,
    
    // Cyrillic
    CYRILLIC: /[\u0400-\u04FF]/,
    
    // Thai
    THAI: /[\u0E00-\u0E7F]/,
    
    // Arabic
    ARABIC: /[\u0600-\u06FF]/,
    
    // Devanagari (Hindi, Sanskrit, etc.)
    DEVANAGARI: /[\u0900-\u097F]/
  };

  // Detect language based on script analysis
  async detectByScript(text: string): Promise<LanguageDetectionResult> {
    try {
      const stats = this.analyzeScripts(text);
      const result = this.determineLanguageFromStats(stats);
      
      this.logger.debug('Script-based detection result', {
        stats,
        result
      });
      
      return result;
    } catch (error) {
      this.logger.error('Script detection error', { error });
      throw error;
    }
  }

  // Analyze text for different scripts
  private analyzeScripts(text: string): Map<string, number> {
    const stats = new Map<string, number>();
    const chars = Array.from(text);
    
    for (const char of chars) {
      for (const [script, regex] of Object.entries(LanguageDetectionFallback.SCRIPTS)) {
        if (regex.test(char)) {
          stats.set(script, (stats.get(script) || 0) + 1);
        }
      }
    }
    
    return stats;
  }

  // Determine language based on script statistics
  private determineLanguageFromStats(stats: Map<string, number>): LanguageDetectionResult {
    if (stats.size === 0) {
      // Default to English for Latin script
      return {
        detectedLang: 'eng',
        confidence: 0.3
      };
    }

    // Find the most common script
    let maxCount = 0;
    let dominantScript = '';
    
    for (const [script, count] of stats.entries()) {
      if (count > maxCount) {
        maxCount = count;
        dominantScript = script;
      }
    }

    // Calculate confidence based on script dominance
    const totalChars = Array.from(stats.values()).reduce((a, b) => a + b, 0);
    const confidence = maxCount / totalChars;

    // Map script to ISO 639-3 language code
    const langCode = this.scriptToLanguage(dominantScript);

    return {
      detectedLang: langCode,
      confidence: Math.min(confidence, 0.8) // Cap confidence at 0.8 for fallback
    };
  }

  // Map script names to ISO 639-3 language codes
  private scriptToLanguage(script: string): string {
    const scriptMap: { [key: string]: string } = {
      JAPANESE: 'jpn',
      KOREAN: 'kor',
      CHINESE: 'cmn',
      CYRILLIC: 'rus', // Default to Russian for Cyrillic
      THAI: 'tha',
      ARABIC: 'ara',
      DEVANAGARI: 'hin' // Default to Hindi for Devanagari
    };

    return scriptMap[script] || 'eng';
  }

  // Statistical analysis for mixed scripts
  async analyzeMixedText(text: string): Promise<LanguageDetectionResult> {
    const stats = this.analyzeScripts(text);
    const scriptCount = stats.size;
    
    if (scriptCount > 1) {
      this.logger.debug('Mixed script detection', {
        scriptCount,
        scripts: Array.from(stats.keys())
      });
      
      // Use more sophisticated analysis for mixed scripts
      return this.handleMixedScripts(stats);
    }
    
    return this.determineLanguageFromStats(stats);
  }

  // Handle text with multiple scripts
  private handleMixedScripts(stats: Map<string, number>): LanguageDetectionResult {
    // Sort scripts by character count
    const sortedScripts = Array.from(stats.entries())
      .sort(([, a], [, b]) => b - a);
    
    const totalChars = Array.from(stats.values()).reduce((a, b) => a + b, 0);
    const [dominantScript, count] = sortedScripts[0];
    
    // Calculate confidence based on script distribution
    const confidence = Math.min(
      (count / totalChars) * 0.8, // Reduce confidence for mixed scripts
      0.7
    );

    return {
      detectedLang: this.scriptToLanguage(dominantScript),
      confidence
    };
  }
}
