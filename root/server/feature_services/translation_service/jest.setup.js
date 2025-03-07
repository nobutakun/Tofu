// This is required for ES modules in Jest
const path = require('path');

module.exports = {
  setupFilesAfterEnv: [path.resolve(__dirname, './src/__tests__/setup.ts')],
  transformIgnorePatterns: [
    'node_modules/(?!(franc|trigram-utils)/)',
  ],
  transform: {
    '^.+\\.tsx?$': ['ts-jest', {
      useESM: true,
      tsconfig: 'tsconfig.json'
    }]
  },
  moduleNameMapper: {
    '^(\\.{1,2}/.*)\\.js$': '$1',
  },
  extensionsToTreatAsEsm: ['.ts']
};
