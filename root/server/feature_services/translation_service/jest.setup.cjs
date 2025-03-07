const { TextEncoder, TextDecoder } = require('util');
global.TextEncoder = TextEncoder;
global.TextDecoder = TextDecoder;

// Handle ESM imports in Jest environment
require('@jest/globals');

// Mock environment variables
process.env.NODE_ENV = 'test';
process.env.PORT = '3001';
process.env.HOST = 'localhost';
process.env.REDIS_HOST = 'localhost';
process.env.REDIS_PORT = '6379';

// Redis mock setup
const mockRedis = {
  on: jest.fn(),
  get: jest.fn(),
  set: jest.fn(),
  quit: jest.fn().mockResolvedValue(undefined),
  status: 'ready',
  disconnect: jest.fn()
};

jest.mock('ioredis', () => {
  return jest.fn(() => mockRedis);
});

// Mock franc for language detection
jest.mock('franc', () => {
  return jest.fn((text) => {
    if (text.includes('日本語')) return 'jpn';
    if (text.includes('한국어')) return 'kor';
    if (text.includes('中文')) return 'cmn';
    if (!text || text.length < 3) return 'und';
    return 'eng';
  });
});

// Silence console output during tests
global.console = {
  ...console,
  log: jest.fn(),
  error: jest.fn(),
  warn: jest.fn(),
  info: jest.fn(),
  debug: jest.fn(),
};

// Cleanup functions
afterEach(() => {
  jest.clearAllMocks();
  jest.clearAllTimers();
});

afterAll(async () => {
  jest.useRealTimers();
  await new Promise(resolve => setTimeout(resolve, 100));
});

// Export mocks for tests
global.__mocks__ = {
  redis: mockRedis
};

// Increase timeout for all tests
jest.setTimeout(30000);
