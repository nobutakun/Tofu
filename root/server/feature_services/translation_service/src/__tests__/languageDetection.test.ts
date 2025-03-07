import { describe, expect, it, beforeEach, jest, beforeAll } from '@jest/globals';
import { LanguageDetectorImpl } from '../services/languageDetection';
import { createLogger } from 'winston';
import type { Logger } from 'winston';
import {
  LanguageDetectionErrorType,
  LanguageDetectionResult,
  LanguageDetectionOptions
} from '../types';

describe('Language Detection Service', () => {
  let detector: LanguageDetectorImpl;
  let logger: Logger;

  beforeAll(() => {
    logger = createLogger({
      transports: []
    });
  });

  beforeEach(() => {
    jest.clearAllMocks();
    detector = new LanguageDetectorImpl(logger);
  });

  describe('Primary Detection', () => {
    it('should detect English text correctly', async () => {
      const result: LanguageDetectionResult = await detector.detectLanguage('This is English text.');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeGreaterThan(0.5);
      expect(result.timestamp).toBeDefined();
    });

    it('should detect Japanese text correctly', async () => {
      const result: LanguageDetectionResult = await detector.detectLanguage('これは日本語のテストです。');
      expect(result.detectedLang).toBe('jpn');
      expect(result.confidence).toBeGreaterThan(0.5);
      expect(result.timestamp).toBeDefined();
    });

    it('should handle empty text appropriately', async () => {
      await expect(detector.detectLanguage('')).rejects.toThrow(
        'Input text must be a non-empty string'
      );
    });

    it('should respect minimum confidence threshold', async () => {
      const text = 'Very short';
      const options: LanguageDetectionOptions = { minConfidence: 0.9 };
      await expect(
        detector.detectLanguage(text, options)
      ).rejects.toThrow('Could not detect language with sufficient confidence');
    });
  });

  describe('Language Support', () => {
    it('should return supported languages', () => {
      const languages = detector.supportedLanguages();
      expect(languages).toContain('eng');
      expect(languages).toContain('jpn');
      expect(languages).toBeInstanceOf(Array);
    });

    it('should map language codes to names correctly', () => {
      expect(detector.getLanguageName('eng')).toBe('English');
      expect(detector.getLanguageName('jpn')).toBe('Japanese');
      expect(detector.getLanguageName('unknown')).toBe('unknown');
    });
  });

  describe('Text Preprocessing', () => {
    it('should handle text preprocessing option', async () => {
      const result: LanguageDetectionResult = await detector.detectLanguage('THIS IS ENGLISH!', {
        textPreprocessing: true
      });
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeGreaterThan(0.5);
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
      const result: LanguageDetectionResult = await detector.detectLanguage('Hi', {
        minConfidence: 0.5
      });
      expect(result.confidence).toBeLessThan(0.7);
    });

    it('should handle mixed scripts with reduced confidence', async () => {
      const result: LanguageDetectionResult = await detector.detectLanguage('Hello こんにちは', {
        minConfidence: 0.5
      });
      expect(result.confidence).toBeLessThan(0.8);
    });
  });

  describe('Edge Cases', () => {
    it('should handle non-ASCII characters', async () => {
      const result: LanguageDetectionResult = await detector.detectLanguage('こんにちは世界！');
      expect(result.detectedLang).toBe('jpn');
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should handle text with numbers and special characters', async () => {
      const result: LanguageDetectionResult = await detector.detectLanguage('Hello 123 !!!');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeGreaterThan(0.5);
    });
  });
});
