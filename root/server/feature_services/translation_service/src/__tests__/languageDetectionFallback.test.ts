import { LanguageDetectionFallback } from '../services/languageDetectionFallback';
import { createLogger } from 'winston';

describe('Language Detection Fallback Service', () => {
  let fallbackDetector: LanguageDetectionFallback;
  const logger = createLogger();

  beforeEach(() => {
    fallbackDetector = new LanguageDetectionFallback(logger);
  });

  describe('Script-based Detection', () => {
    it('should detect Japanese text correctly', async () => {
      const result = await fallbackDetector.detectByScript('こんにちは世界');
      expect(result.detectedLang).toBe('jpn');
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should detect Korean text correctly', async () => {
      const result = await fallbackDetector.detectByScript('안녕하세요');
      expect(result.detectedLang).toBe('kor');
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should detect Chinese text correctly', async () => {
      const result = await fallbackDetector.detectByScript('你好世界');
      expect(result.detectedLang).toBe('cmn');
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should detect Thai text correctly', async () => {
      const result = await fallbackDetector.detectByScript('สวัสดีชาวโลก');
      expect(result.detectedLang).toBe('tha');
      expect(result.confidence).toBeGreaterThan(0.5);
    });
  });

  describe('Mixed Script Analysis', () => {
    it('should handle mixed Japanese and English text', async () => {
      const result = await fallbackDetector.analyzeMixedText('Hello こんにちは World');
      expect(result.confidence).toBeLessThan(0.7); // Lower confidence for mixed text
    });

    it('should identify dominant script in mixed text', async () => {
      const result = await fallbackDetector.analyzeMixedText('こんにちは Hello こんばんは おはよう');
      expect(result.detectedLang).toBe('jpn'); // Japanese should be dominant
      expect(result.confidence).toBeGreaterThan(0.5);
    });

    it('should handle equal distribution of scripts', async () => {
      const result = await fallbackDetector.analyzeMixedText('Hello World こんにちは世界');
      expect(result.confidence).toBeLessThan(0.6); // Very low confidence for equal distribution
    });
  });

  describe('Edge Cases', () => {
    it('should handle empty text', async () => {
      await expect(fallbackDetector.detectByScript('')).rejects.toThrow();
    });

    it('should handle non-standard characters', async () => {
      const result = await fallbackDetector.detectByScript('Hello™®©');
      expect(result.detectedLang).toBe('eng');
      expect(result.confidence).toBeLessThan(0.5);
    });

    it('should handle numeric and special characters', async () => {
      const result = await fallbackDetector.detectByScript('12345 !@#$%');
      expect(result.confidence).toBeLessThan(0.4);
    });

    it('should handle very short text', async () => {
      const result = await fallbackDetector.detectByScript('あ');
      expect(result.confidence).toBeLessThan(0.6);
    });
  });

  describe('Error Handling', () => {
    it('should handle null input', async () => {
      // @ts-ignore - Testing invalid input
      await expect(fallbackDetector.detectByScript(null)).rejects.toThrow();
    });

    it('should handle undefined input', async () => {
      // @ts-ignore - Testing invalid input
      await expect(fallbackDetector.detectByScript(undefined)).rejects.toThrow();
    });
  });

  afterEach(() => {
    jest.clearAllMocks();
  });
});
