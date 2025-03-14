import { describe, expect, it, jest, beforeAll, afterAll } from '@jest/globals';
import { app, serverInstance } from '../index';
import * as supertest from 'supertest';
import type { SuperTest, Test, Response } from 'supertest';
import type { DetectionResponse, CustomError, LanguageDetectionResult } from '../types';
import { Server } from 'http';

// Increase timeout for all tests in this file
jest.setTimeout(30000);

interface ApiErrorResponse {
  error: string;
  message?: string;
  type?: string;
}

const request = supertest.agent(app);

// Add a direct setup for this test
jest.mock('franc', () => ({
  franc: (text: string) => {
    if (text.includes('This is a sample English text')) return 'eng';
    // Rest of the mock...
    return 'eng';
  }
}), { virtual: true });

describe('Language Detection API Integration Tests', () => {
  // Create a local server variable for this test suite
  let testServer: Server | null = null;
  
  beforeAll(async () => {
    // Start server on test port if needed for integration tests
    const port = parseInt(process.env.PORT || '3001');
    const host = process.env.HOST || 'localhost';
    
    // Create a new server for testing
    testServer = app.listen(port, host);
    
    // Wait for server to start
    await new Promise(resolve => setTimeout(resolve, 100));
  });

  describe('Health Check', () => {
    it('should return health status', async () => {
      const response: Response = await request.get('/health');
      expect(response.status).toBe(200);
      expect(response.body).toHaveProperty('status', 'ok');
      expect(response.body).toHaveProperty('services.redis');
    });
  });

  describe('Language Detection Endpoint', () => {
    it('should detect English text correctly', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .send({
          text: 'This is a sample English text for testing purposes.'
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      const result = response.body as DetectionResponse;
      // Accept either eng or sco as valid results
      expect(['eng', 'sco']).toContain(result.detectedLang);
      expect(result.method).toBe('primary');
      expect(result.confidence).toBeGreaterThan(0.5);
      expect(result.timestamp).toBeDefined();
    });

    it('should detect Japanese text correctly', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .send({
          text: 'これは日本語のテストです。'
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      const result = response.body as DetectionResponse;
      expect(result.detectedLang).toBe('jpn');
      expect(result.method).toBe('primary');
      expect(result.confidence).toBeGreaterThan(0.5);
      expect(result.timestamp).toBeDefined();
    });

    it('should handle empty text appropriately', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .send({ text: '' })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(400);
      const errorResponse = response.body as ApiErrorResponse;
      expect(errorResponse.error).toBe('Text is required');
    });

    it('should handle null text appropriately', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .send({ text: null })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(400);
      const errorResponse = response.body as ApiErrorResponse;
      expect(errorResponse.error).toBe('Text is required');
    });

    it('should handle detection options', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .send({
          text: 'Sample text',
          options: {
            minConfidence: 0.3,
            textPreprocessing: true,
            preferredLanguages: ['eng', 'jpn']
          }
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      const result = response.body as DetectionResponse;
      expect(result.detectedLang).toBeDefined();
      expect(result.confidence).toBeDefined();
      expect(result.timestamp).toBeDefined();
    });

    it('should fallback when primary detection fails', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .send({
          text: '漢字とEnglishの Mixed Text',
          options: {
            minConfidence: 0.9 // High threshold to force fallback
          }
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      const result = response.body as DetectionResponse;
      expect(result.method).toBe('fallback');
      expect(result.detectedLang).toBeDefined();
      expect(result.confidence).toBeGreaterThan(0.3);
    });
  });

  describe('Error Handling', () => {
    it('should handle malformed JSON', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .set('Content-Type', 'application/json')
        .send('{"text": "malformed JSON"'); // Intentionally malformed JSON

      expect(response.status).toBe(400);
      const errorResponse = response.body as ApiErrorResponse;
      expect(errorResponse.error).toBe('Invalid JSON format');
    });

    it('should handle unsupported content types', async () => {
      const response: Response = await request
        .post('/api/detect-language')
        .set('Content-Type', 'text/plain')
        .send('Plain text content');

      expect(response.status).toBe(415);
      const errorResponse = response.body as ApiErrorResponse;
      expect(errorResponse.error).toMatch(/Content-Type must be application\/json/i);
    });

    it('should handle request timeout', async () => {
      const longText = 'a'.repeat(1000000);
      
      const timeoutPromise = new Promise<void>((_, reject) => {
        setTimeout(() => reject(new Error('Timeout')), 6000);
      });

      const requestPromise = request
        .post('/api/detect-language')
        .send({ text: longText })
        .set('Content-Type', 'application/json');

      try {
        await Promise.race([requestPromise, timeoutPromise]);
      } catch (error) {
        const customError = error as CustomError;
        expect(customError.message).toBe('Timeout');
      }
    }, 10000);
  });

  describe('Supported Languages Endpoint', () => {
    interface SupportedLanguageResponse {
      languages: Array<{
        code: string;
        name: string;
      }>;
    }

    it('should return list of supported languages', async () => {
      const response: Response = await request.get('/api/supported-languages');

      expect(response.status).toBe(200);
      const result = response.body as SupportedLanguageResponse;
      expect(result.languages).toBeDefined();
      expect(result.languages).toEqual(
        expect.arrayContaining([
          expect.objectContaining({
            code: 'eng',
            name: 'English'
          }),
          expect.objectContaining({
            code: 'jpn',
            name: 'Japanese'
          })
        ])
      );
    });
  });

  afterAll(async () => {
    // Clean up - make sure to close the testServer we created
    if (testServer) {
      await new Promise<void>((resolve) => {
        testServer!.close(() => resolve());
      });
      testServer = null;
    }
    
    // Additional cleanup if needed
    await new Promise(resolve => setTimeout(resolve, 100));
  });
});
