'use strict';

/*
 * 用 generate-evb 生成 Enigma Virtual Box 工程文件（*.evb），再调用 enigmavbconsole
 * 把 dist 目录下的所有运行库与插件封装进主程序，产出一个单文件 exe。
 *
 * 用法：
 *   npm run pack                       # 生成 evb 并打包
 *   npm run pack -- --evb-only         # 只生成 evb，不打包
 *
 * enigmavbconsole.exe 的查找顺序：环境变量 ENIGMAVB_CONSOLE -> 默认安装路径。
 */

const { execFileSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const generateEvb = require('generate-evb');

const root = path.resolve(__dirname, '..', '..');
const distDir = path.join(root, 'dist');
const packDir = path.join(root, 'build', 'pack');

const inputExe = path.join(distDir, 'autoLoginTool.exe');   // 宿主程序（dist 下未被封装的原始 exe）
const projectFile = path.join(packDir, 'autoLoginTool.evb');
const outputExe = path.join(packDir, 'AutoLoginQatarTool.exe'); // 封装后的单文件程序

const consoleCandidates = [
    process.env.ENIGMAVB_CONSOLE,
    'D:\\tool\\Enigma Virtual Box\\enigmavbconsole.exe'
].filter(Boolean);

function findConsole() {
    return consoleCandidates.find((candidate) => fs.existsSync(candidate)) || null;
}

if (!fs.existsSync(inputExe)) {
    console.error(`找不到待封装的主程序：${inputExe}`);
    process.exit(1);
}

fs.mkdirSync(packDir, { recursive: true });

generateEvb(projectFile, inputExe, outputExe, distDir, {
    // 宿主 exe 自身不需要再作为虚拟文件打进去
    filter: (fullPath, name, isDir) => {
        if (isDir) return true;
        return path.resolve(fullPath) !== path.resolve(inputExe);
    },
    evbOptions: {
        compressFiles: true,
        deleteExtractedOnExit: true
    }
});
console.log(`已生成工程文件：${projectFile}`);

if (process.argv.includes('--evb-only')) {
    console.log(`如需手动打包：enigmavbconsole.exe "${projectFile}"`);
    process.exit(0);
}

const consoleExe = findConsole();
if (!consoleExe) {
    console.error('未找到 enigmavbconsole.exe，请设置环境变量 ENIGMAVB_CONSOLE 后重试');
    process.exit(1);
}

execFileSync(consoleExe, [projectFile], { stdio: 'inherit' });

if (!fs.existsSync(outputExe)) {
    console.error(`封装失败，未生成：${outputExe}`);
    process.exit(1);
}
console.log(`已生成单文件程序：${outputExe}（${(fs.statSync(outputExe).size / 1024 / 1024).toFixed(2)} MB）`);
