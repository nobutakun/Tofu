// Language detection interfaces
export interface LanguageDetectionResult {
  detectedLang: string;       // ISO 639-3 language code
  confidence: number;         // Confidence score between 0 and 1
  timestamp?: number;         // Optional timestamp of detection
}

export interface LanguageDetectionOptions {
  minConfidence?: number;     // Minimum confidence threshold
  preferredLanguages?: string[]; // List of preferred languages to check first
  textPreprocessing?: boolean;   // Whether to preprocess text before detection
}

export interface LanguageDetectionService {
  detectLanguage(text: string, options?: LanguageDetectionOptions): Promise<LanguageDetectionResult>;
  supportedLanguages(): string[];  // Returns list of supported language codes
  getLanguageName(code: string): string; // Convert language code to full name
}

// Error types
export enum LanguageDetectionErrorType {
  INVALID_INPUT = 'INVALID_INPUT',
  UNSUPPORTED_LANGUAGE = 'UNSUPPORTED_LANGUAGE',
  CONFIDENCE_TOO_LOW = 'CONFIDENCE_TOO_LOW',
  INTERNAL_ERROR = 'INTERNAL_ERROR'
}

export class LanguageDetectionError extends Error {
  constructor(
    public type: LanguageDetectionErrorType,
    message: string
  ) {
    super(message);
    this.name = 'LanguageDetectionError';
  }
}
