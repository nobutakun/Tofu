export interface LanguageDetectionOptions {
  minConfidence?: number;
  preferredLanguages?: string[];
  textPreprocessing?: boolean;
}

export interface LanguageDetectionResult {
  detectedLang: string;
  confidence: number;
  timestamp?: number;
  method?: 'primary' | 'fallback';
}

export enum LanguageDetectionErrorType {
  INVALID_INPUT = 'INVALID_INPUT',
  CONFIDENCE_TOO_LOW = 'CONFIDENCE_TOO_LOW',
  INTERNAL_ERROR = 'INTERNAL_ERROR'
}

export class LanguageDetectionError extends Error {
  constructor(
    public readonly type: LanguageDetectionErrorType,
    message: string
  ) {
    super(message);
    this.name = 'LanguageDetectionError';
  }
}

export interface LanguageDetectionService {
  detectLanguage(
    text: string,
    options?: LanguageDetectionOptions
  ): Promise<LanguageDetectionResult>;
  supportedLanguages(): string[];
  getLanguageName(code: string): string;
}

export interface DetectionResponse extends LanguageDetectionResult {
  error?: {
    message: string;
    type?: string;
  };
}

// Custom error type for type checking
export interface CustomError extends Error {
  message: string;
}
