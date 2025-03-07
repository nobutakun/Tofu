import { LanguageDetectorImpl } from '../services/languageDetection';
import { createLogger } from 'winston';
import { LanguageDetectionErrorType } from '../types';

describe('Language Detection Service', () => {
  let detector: LanguageDetectorImpl;
  const logger = createLogger();

  beforeEach(() => {
    detector = new LanguageDetectorImpl(logger);
  });

  describe('Primary Detection', () => {
    it('should detect English text correctly', async () => {
      const result = await detector.detectLanguage('This is English text.');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should detect Japanese text correctly', async () => {
      const result = await detector.detectLanguage('これは日本語のテストです。');
      expect(result.detectedLang).toBe('jpn');
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should handle empty text appropriately', async () => {
      await expect(detector.detectLanguage('')).rejects.toThrow(
        'Input text must be a non-empty string'
      );
    });

    it('should respect minimum confidence threshold', async () => {
      const text = 'Very short';
      await expect(
        detector.detectLanguage(text, { minConfidence: 0.9 })
      ).rejects.toThrow('Could not detect language with sufficient confidence');
    });
  });

  describe('Language Support', () => {
    it('should return supported languages', () => {
      const languages = detector.supportedLanguages();
      expect(languages).toContain('eng');
      expect(languages).toContain('jpn');
    });

    it('should map language codes to names correctly', () => {
      expect(detector.getLanguageName('eng')).toBe('English');
      expect(detector.getLanguageName('jpn')).toBe('Japanese');
    });
  });

  describe('Text Preprocessing', () => {
    it('should handle text preprocessing option', async () => {
      const result = await detector.detectLanguage('THIS IS ENGLISH!', {
        textPreprocessing: true
      });
      expect(result.detectedLang).toBe('eng');
    });
  });

  describe('Error Handling', () => {
    it('should handle invalid input types', async () => {
      // @ts-ignore - Testing invalid input
      await expect(detector.detectLanguage(null)).rejects.toThrow(
        'Input text must be a non-empty string'
      );
    });

    it('should handle very short text appropriately', async () => {
      const result = await detector.detectLanguage('Hi', {
        minConfidence: 0.5
      });
      expect(result.confidence).toBeLessThan(0.7); // Allow lower confidence for very short text
    });

    it('should handle mixed scripts', async () => {
      const result = await detector.detectLanguage('Hello こんにちは', {
        minConfidence: 0.5
      });
      expect(result.confidence).toBeLessThan(0.8); // Mixed scripts should have lower confidence
    });
  });

  afterEach(() => {
    jest.clearAllMocks();
  });
});
