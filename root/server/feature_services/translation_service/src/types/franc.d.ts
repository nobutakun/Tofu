declare module 'franc' {
  /**
   * Detect the language of a given text
   * @param text - The text to analyze
   * @param options - Optional configuration options
   * @returns ISO 639-3 code of the detected language or 'und' for undefined
   */
  function franc(text: string, options?: {
    /**
     * Minimum length of text to accept
     */
    minLength?: number;
    /**
     * Allow only specific languages
     */
    whitelist?: string[];
    /**
     * Disallow specific languages
     */
    blacklist?: string[];
  }): string;

  export = franc;
}
