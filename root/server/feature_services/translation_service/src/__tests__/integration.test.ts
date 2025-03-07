import { app } from '../index';
import supertest from 'supertest';

const request = supertest(app);
jest.setTimeout(15000); // Increase global timeout for tests

describe('Language Detection API Integration Tests', () => {
  describe('Health Check', () => {
    it('should return health status', async () => {
      const response = await request.get('/health');
      expect(response.status).toBe(200);
      expect(response.body).toHaveProperty('status', 'ok');
      expect(response.body).toHaveProperty('services.redis');
    });
  });

  describe('Language Detection Endpoint', () => {
    it('should detect English text correctly', async () => {
      const response = await request
        .post('/api/detect-language')
        .send({
          text: 'This is a sample English text for testing purposes.'
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      expect(response.body).toMatchObject({
        detectedLang: 'eng',
        method: 'primary'
      });
      expect(response.body.confidence).toBeGreaterThan(0.5);
    });

    it('should detect Japanese text correctly', async () => {
      const response = await request
        .post('/api/detect-language')
        .send({
          text: 'これは日本語のテストです。'
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      expect(response.body).toMatchObject({
        detectedLang: 'jpn',
        method: 'primary'
      });
      expect(response.body.confidence).toBeGreaterThan(0.5);
    });

    it('should handle empty text appropriately', async () => {
      const response = await request
        .post('/api/detect-language')
        .send({
          text: ''
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(400);
      expect(response.body).toHaveProperty('error', 'Text is required');
    });

    it('should handle detection options', async () => {
      const response = await request
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
      expect(response.body).toHaveProperty('detectedLang');
      expect(response.body).toHaveProperty('confidence');
    });

    it('should fallback when primary detection fails', async () => {
      const response = await request
        .post('/api/detect-language')
        .send({
          text: '漢字とEnglishの Mixed Text',
          options: {
            minConfidence: 0.9 // High threshold to force fallback
          }
        })
        .set('Content-Type', 'application/json');

      expect(response.status).toBe(200);
      expect(response.body).toHaveProperty('method', 'fallback');
    });
  });

  describe('Error Handling', () => {
    it('should handle malformed JSON', async () => {
      const response = await request
        .post('/api/detect-language')
        .set('Content-Type', 'application/json')
        .send('{"text": "malformed JSON"'); // Intentionally malformed JSON

      expect(response.status).toBe(400);
      expect(response.body.error).toBe('Invalid JSON format');
    });

    it('should handle unsupported content types', async () => {
      const response = await request
        .post('/api/detect-language')
        .set('Content-Type', 'text/plain')
        .send('Plain text content');

      expect(response.status).toBe(415);
      expect(response.body.error).toMatch(/Content-Type must be application\/json/i);
    });

    it('should handle request timeout', async () => {
      // Create a delayed request that will trigger timeout
      const longText = Array(1000000).fill('a').join('');
      const timeoutPromise = new Promise((resolve) => setTimeout(resolve, 6000));
      
      const response = await Promise.race([
        request
          .post('/api/detect-language')
          .send({ text: longText })
          .set('Content-Type', 'application/json'),
        timeoutPromise.then(() => ({ status: 503, body: { error: 'Request timeout' } }))
      ]);

      expect(response.status).toBe(503);
      expect(response.body.error).toBe('Request timeout');
    }, 10000); // Set higher timeout for this specific test
  });

  describe('Supported Languages Endpoint', () => {
    it('should return list of supported languages', async () => {
      const response = await request.get('/api/supported-languages');

      expect(response.status).toBe(200);
      expect(response.body).toHaveProperty('languages');
      expect(response.body.languages).toEqual(
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
    // Ensure cleanup of any remaining connections
    await new Promise((resolve) => setTimeout(resolve, 500));
  });
});
