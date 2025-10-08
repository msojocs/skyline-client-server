// rollup.config.js
import resolve from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import typescript from '@rollup/plugin-typescript';
import json from '@rollup/plugin-json'
import terser from '@rollup/plugin-terser';
import { InputPluginOption, RollupOptions } from "rollup";
import fs from 'fs'
import path from 'path';

const outputDir = process.env['outputDir'] || 'dist/'
const isDev = process.env.ENVIRONMENT !== 'production'
const options: RollupOptions[] = [
  {
    input: 'src/server.ts',
    output: {
      strict: false,
      // dir: 'D:/down/nwjs-sdk-v0.54.1-win-x64/package.nw',
      dir: '../nwjs/webview',
      // file: 'server.js',
      format: 'cjs',
      // banner: (chunk) => {
      //   // console.log('chunk:', chunk)
      //   return fs.readFileSync('./tools/prepend.js').toString()
      // },
    },
    onwarn: function (warning, handler) {
      // Skip certain warnings

      // should intercept ... but doesn't in some rollup versions
      if (warning.code === 'THIS_IS_UNDEFINED') { return; }
      if (warning.code === 'CIRCULAR_DEPENDENCY') { return; }

      // console.warn everything else
      handler(warning);
    },
    plugins: [
      resolve({
        // 将自定义选项传递给解析插件
        moduleDirectories: ['node_modules'],
        preferBuiltins: true,
      }),
      commonjs({
        include: /node_modules/,
        requireReturnsDefault: 'auto', // <---- this solves default issue
      }),
      typescript(),
      json(),
      // 压缩
      !isDev ? terser() : null,
    ],
    // 指出哪些模块应该视为外部模块
    external: ['nw', 'module'],
  },
];
export default options