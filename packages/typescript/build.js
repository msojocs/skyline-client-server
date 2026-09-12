async function buildServers() {
  const { build } = await import('vite');
  const watch = process.argv.includes('--watch');
  // Separate Rollup graphs inline shared modules into each deployable entry.
  for (const mode of ['renderer', 'main']) {
    await build({
      root: __dirname,
      mode,
      build: { watch: watch ? {} : null },
    });
  }
}

buildServers().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
