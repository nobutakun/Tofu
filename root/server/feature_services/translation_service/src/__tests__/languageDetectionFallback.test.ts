import { describe, expect, it, beforeEach, jest } from '@jest/globals';
import { LanguageDetectionFallback } from '../services/languageDetectionFallback';
import { createLogger } from 'winston';
import type { Logger } from 'winston';
import type { LanguageDetectionResult } from '../types';

describe('Language Detection Fallback Service', () => {
  let fallbackDetector: LanguageDetectionFallback;
  let logger: Logger;

  beforeEach(() => {
    logger = createLogger({
      transports: []
    });
    fallbackDetector = new LanguageDetectionFallback(logger);
    jest.clearAllMocks();
  });

  describe('Script-based Detection', () => {
    it('should detect Latin script as English', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('Hello world');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeDefined();
      expect(result.confidence).toBeGreaterThan(0.3);
    });

    it('should detect Japanese script', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('こんにちは');
      expect(result.detectedLang).toBe('jpn');
      expect(result.confidence).toBeDefined();
      expect(result.confidence).toBeGreaterThan(0.3);
    });

    it('should detect Chinese script', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('你好世界');
      expect(result.detectedLang).toBe('cmn');
      expect(result.confidence).toBeDefined();
      expect(result.confidence).toBeGreaterThan(0.3);
    });

    it('should detect Korean script', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('안녕하세요');
      expect(result.detectedLang).toBe('kor');
      expect(result.confidence).toBeDefined();
      expect(result.confidence).toBeGreaterThan(0.3);
    });
  });

  describe('Mixed Script Handling', () => {
    it('should handle mixed scripts with Latin preference', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('Hello こんにちは');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeLessThan(0.8);
    });

    it('should handle mixed scripts with CJK preference', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('こんにちは Hello');
      expect(['jpn', 'cmn', 'kor']).toContain(result.detectedLang);
      expect(result.confidence).toBeLessThan(0.8);
    });
  });

  describe('Error Handling', () => {
    it('should handle empty input', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('');
      expect(result.detectedLang).toBe('eng'); // Default to English for empty input
      expect(result.confidence).toBeLessThan(0.5);
    });

    it('should handle numeric-only input', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('12345');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeLessThan(0.5);
    });

    it('should handle special characters', async () => {
      const result: LanguageDetectionResult = await fallbackDetector.detectByScript('!@#$%^&*()');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeLessThan(0.5);
    });
  });

  describe('Confidence Levels', () => {
    it('should assign higher confidence to pure scripts', async () => {
      const pureResult: LanguageDetectionResult = await fallbackDetector.detectByScript('こんにちは世界');
      const mixedResult: LanguageDetectionResult = await fallbackDetector.detectByScript('こんにちは World');
      expect(pureResult.confidence).toBeGreaterThan(mixedResult.confidence);
    });

    it('should handle longer text with higher confidence', async () => {
      const shortResult: LanguageDetectionResult = await fallbackDetector.detectByScript('Hello');
      const longResult: LanguageDetectionResult = await fallbackDetector.detectByScript('Hello this is a much longer text for testing confidence levels');
      expect(longResult.confidence).toBeGreaterThan(shortResult.confidence);
    });
  });
});
