import { defineConfig } from 'vite';
import { resolve } from 'node:path';

const packageRoot = __dirname;

export default defineConfig(() => ({
  build: {
    // Keep the output layout consumed by the Electron package and release scripts.
    outDir: resolve(packageRoot, '../electron'),
    emptyOutDir: false,
    minify: process.env.ENVIRONMENT === 'production' ? ('esbuild' as const) : false,
    rollupOptions: {
      input: {
        server: resolve(packageRoot, 'src/render-server.ts'),
        main: resolve(packageRoot, '../electron/main.ts'),
        'main-rpc': resolve(packageRoot, '../electron/main-rpc.ts'),
      },
      external: [
        'electron',
        'module',
        'skyline-server/render-server.node',
        /^node:/,
      ],
      // Keep every public export available when main-rpc.js is required by
      // native tests or by another Electron entry.
      preserveEntrySignatures: 'strict' as const,
      output: {
        format: 'cjs' as const,
        entryFileNames: '[name].js',
        chunkFileNames: 'chunks/[name]-[hash].js',
        exports: 'auto' as const,
      },
    },
  },
}));
