import { 
  LanguageDetectionService, 
  LanguageDetectionResult, 
  LanguageDetectionOptions,
  LanguageDetectionError,
  LanguageDetectionErrorType
} from '../types';
import franc from 'franc';
import { Logger } from 'winston';

const LANGUAGE_NAMES: { [key: string]: string } = {
  eng: 'English',
  jpn: 'Japanese',
  cmn: 'Chinese',
  kor: 'Korean',
  rus: 'Russian',
  spa: 'Spanish',
  fra: 'French',
  deu: 'German',
};

export class LanguageDetectorImpl implements LanguageDetectionService {
  private readonly DEFAULT_MIN_CONFIDENCE = 0.5;
  private readonly MIN_TEXT_LENGTH = 10;
  private readonly VERY_SHORT_TEXT_LENGTH = 5;

  constructor(private logger: Logger) {
    this.logger = logger.child({ service: 'LanguageDetection' });
  }

  async detectLanguage(
    text: string, 
    options: LanguageDetectionOptions = {}
  ): Promise<LanguageDetectionResult> {
    try {
      // Input validation
      if (!text || typeof text !== 'string') {
        throw new LanguageDetectionError(
          LanguageDetectionErrorType.INVALID_INPUT,
          'Input text must be a non-empty string'
        );
      }

      // Text preprocessing if enabled
      if (options.textPreprocessing) {
        text = this.preprocessText(text);
      }

      // Text length check
      if (text.length < this.MIN_TEXT_LENGTH) {
        this.logger.warn('Text too short for reliable detection', {
          textLength: text.length,
          minLength: this.MIN_TEXT_LENGTH
        });
      }

      const result = await this.primaryDetection(text, options);
      
      this.logger.debug('Language detection completed', {
        detectedLang: result.detectedLang,
        confidence: result.confidence
      });

      return {
        ...result,
        timestamp: Date.now()
      };
    } catch (error) {
      if (error instanceof LanguageDetectionError) {
        throw error;
      }
      
      this.logger.error('Language detection error', { error });
      throw new LanguageDetectionError(
        LanguageDetectionErrorType.INTERNAL_ERROR,
        'Internal language detection error'
      );
    }
  }

  supportedLanguages(): string[] {
    return Object.keys(LANGUAGE_NAMES);
  }

  getLanguageName(code: string): string {
    return LANGUAGE_NAMES[code] || code;
  }

  private async primaryDetection(
    text: string,
    options: LanguageDetectionOptions
  ): Promise<LanguageDetectionResult> {
    const francOptions: any = {
      minLength: 1,
      only: options.preferredLanguages
    };

    const detectedLang = franc(text, francOptions);
    const minConfidence = options.minConfidence || this.DEFAULT_MIN_CONFIDENCE;

    if (detectedLang === 'und') {
      throw new LanguageDetectionError(
        LanguageDetectionErrorType.CONFIDENCE_TOO_LOW,
        'Could not detect language with sufficient confidence'
      );
    }

    // Calculate confidence based on text characteristics
    const baseConfidence = this.estimateBaseConfidence(text);
    const scriptConfidence = this.estimateScriptConfidence(text, detectedLang);
    const lengthPenalty = this.calculateLengthPenalty(text);

    const finalConfidence = Math.min(
      baseConfidence * scriptConfidence * (1 - lengthPenalty),
      0.99
    );

    if (finalConfidence < minConfidence) {
      throw new LanguageDetectionError(
        LanguageDetectionErrorType.CONFIDENCE_TOO_LOW,
        'Could not detect language with sufficient confidence'
      );
    }

    return {
      detectedLang,
      confidence: finalConfidence
    };
  }

  private preprocessText(text: string): string {
    return text
      .trim()
      .toLowerCase()
      .replace(/\s+/g, ' ')
      .replace(/[^\p{L}\s]/gu, '');
  }

  private estimateBaseConfidence(text: string): number {
    const length = text.length;
    if (length >= 100) return 0.95;
    if (length >= 50) return 0.90;
    if (length >= 20) return 0.85;
    if (length >= this.MIN_TEXT_LENGTH) return 0.75;
    if (length >= this.VERY_SHORT_TEXT_LENGTH) return 0.65;
    return 0.60;
  }

  private estimateScriptConfidence(text: string, detectedLang: string): number {
    const hasMultipleScripts = this.hasMultipleScripts(text);
    if (hasMultipleScripts) {
      return 0.7; // Reduce confidence for mixed scripts
    }

    // Higher confidence for matching script and language
    const matchesExpectedScript = this.matchesExpectedScript(text, detectedLang);
    return matchesExpectedScript ? 1.0 : 0.8;
  }

  private calculateLengthPenalty(text: string): number {
    if (text.length < this.VERY_SHORT_TEXT_LENGTH) return 0.3;
    if (text.length < this.MIN_TEXT_LENGTH) return 0.2;
    return 0;
  }

  private hasMultipleScripts(text: string): boolean {
    const hasLatin = /[a-zA-Z]/.test(text);
    const hasCJK = /[\u4E00-\u9FFF\u3040-\u309F\u30A0-\u30FF\uAC00-\uD7AF]/.test(text);
    return hasLatin && hasCJK;
  }

  private matchesExpectedScript(text: string, detectedLang: string): boolean {
    switch (detectedLang) {
      case 'jpn':
        return /[\u3040-\u309F\u30A0-\u30FF]/.test(text);
      case 'cmn':
        return /[\u4E00-\u9FFF]/.test(text);
      case 'kor':
        return /[\uAC00-\uD7AF]/.test(text);
      default:
        return /[a-zA-Z]/.test(text);
    }
  }
}
