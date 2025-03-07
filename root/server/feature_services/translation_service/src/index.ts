import express, { Express, Request, Response, NextFunction } from 'express';
import { createServer } from 'http';
import { config } from './config';
import * as winston from 'winston';
import Redis from 'ioredis';
import { LanguageDetectorImpl } from './services/languageDetection';
import { LanguageDetectionFallback } from './services/languageDetectionFallback';
import { LanguageDetectionOptions } from './types';

// Initialize logger
const logger = winston.createLogger({
  level: 'info',
  format: winston.format.combine(
    winston.format.timestamp(),
    winston.format.json()
  ),
  transports: [
    new winston.transports.Console(),
    new winston.transports.File({ filename: 'error.log', level: 'error' }),
    new winston.transports.File({ filename: 'combined.log' })
  ]
});

// Initialize Redis client
const redisClient = new Redis({
  host: config.redis.host,
  port: config.redis.port,
  password: config.redis.password,
  retryStrategy: (times) => {
    const delay = Math.min(times * 50, 2000);
    return delay;
  }
});

redisClient.on('error', (err) => {
  logger.error('Redis connection error:', err);
});

// Initialize Express app
const app: Express = express();
const server = createServer(app);

// Initialize services
const languageDetector = new LanguageDetectorImpl(logger);
const fallbackDetector = new LanguageDetectionFallback(logger);

// Request timeout middleware
const timeout = (ms: number) => (req: Request, res: Response, next: NextFunction) => {
  res.setTimeout(ms, () => {
    res.status(503).json({ error: 'Request timeout' });
  });
  next();
};

// JSON parsing error handler
const jsonParsingError = (err: Error, req: Request, res: Response, next: NextFunction) => {
  if (err instanceof SyntaxError && 'body' in err) {
    return res.status(400).json({ error: 'Invalid JSON format' });
  }
  next(err);
};

// Middleware
app.use(express.json({ verify: (_req, _res, buf) => { JSON.parse(buf.toString()); } }));
app.use(jsonParsingError);
app.use(express.urlencoded({ extended: true }));
app.use(timeout(5000)); // 5 second timeout

// Request logging middleware
app.use((req, res, next) => {
  logger.info('Incoming request', {
    method: req.method,
    path: req.path,
    query: req.query,
    body: req.body
  });
  next();
});

// Content type validation middleware
app.use((req, res, next) => {
  if (req.method === 'POST' && !req.is('application/json')) {
    return res.status(415).json({
      error: 'Content-Type must be application/json'
    });
  }
  next();
});

// Health check endpoint
app.get('/health', (_req: Request, res: Response) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date().toISOString(),
    services: {
      redis: redisClient.status === 'ready'
    }
  });
});

// Language detection endpoint
app.post('/api/detect-language', async (req: Request, res: Response) => {
  try {
    const { text, options } = req.body;
    
    if (!text) {
      return res.status(400).json({
        error: 'Text is required'
      });
    }

    const detectionOptions: LanguageDetectionOptions = {
      minConfidence: options?.minConfidence || 0.5,
      preferredLanguages: options?.preferredLanguages,
      textPreprocessing: options?.textPreprocessing
    };

    try {
      const result = await languageDetector.detectLanguage(text, detectionOptions);
      return res.json({
        ...result,
        method: 'primary'
      });
    } catch (error) {
      logger.warn('Primary detection failed, trying fallback', { error });
      
      const fallbackResult = await fallbackDetector.detectByScript(text);
      return res.json({
        ...fallbackResult,
        method: 'fallback'
      });
    }
  } catch (error) {
    logger.error('Language detection error', { error });
    return res.status(500).json({
      error: 'Internal server error',
      message: error instanceof Error ? error.message : 'Unknown error'
    });
  }
});

// Get supported languages endpoint
app.get('/api/supported-languages', (_req: Request, res: Response) => {
  const languages = languageDetector.supportedLanguages();
  res.json({
    languages: languages.map(code => ({
      code,
      name: languageDetector.getLanguageName(code)
    }))
  });
});

// Error handling middleware
app.use((err: Error, _req: Request, res: Response, _next: NextFunction) => {
  logger.error('Unhandled error:', err);
  res.status(500).json({
    error: 'Internal server error',
    timestamp: new Date().toISOString()
  });
});

// Start server
const serverInstance = server.listen(config.server.port, config.server.host, () => {
  logger.info(`Translation service listening on ${config.server.host}:${config.server.port}`);
});

// Graceful shutdown
const cleanup = () => {
  serverInstance.close(() => {
    logger.info('Server closed');
    redisClient.quit();
    process.exit(0);
  });

  // Force close after timeout
  setTimeout(() => {
    logger.error('Could not close connections in time, forcefully shutting down');
    process.exit(1);
  }, 10000);
};

process.on('SIGTERM', cleanup);
process.on('SIGINT', cleanup);

export { app, redisClient, logger, serverInstance };
