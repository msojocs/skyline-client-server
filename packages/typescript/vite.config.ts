import { defineConfig } from 'vite';
import { resolve } from 'node:path';

const packageRoot = __dirname;

export default defineConfig(({ mode }) => ({
  build: {
    // Keep the output layout consumed by the Electron package and release scripts.
    outDir: resolve(packageRoot, '../electron'),
    emptyOutDir: false,
    minify: process.env.ENVIRONMENT === 'production' ? ('esbuild' as const) : false,
    rollupOptions: {
      input: {
        [mode === 'main' ? 'main-server' : 'render-server']: resolve(
          packageRoot, mode === 'main' ? 'src/main-server.ts' : 'src/render-server.ts',
        ),
      },
      external: [
        'electron',
        'module',
        'skyline-server/render-server.node',
        /^node:/,
      ],
      // Keep every public export available when the bundled main server is
      // required by native tests or by another Electron entry.
      preserveEntrySignatures: 'strict' as const,
      output: {
        format: 'cjs' as const,
        inlineDynamicImports: true,
        entryFileNames: '[name].js',
        chunkFileNames: 'chunks/[name]-[hash].js',
        exports: 'auto' as const,
      },
    },
  },
}));
