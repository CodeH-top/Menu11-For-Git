import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import sharp from 'sharp';

const toolDirectory = dirname(fileURLToPath(import.meta.url));
const repositoryRoot = resolve(toolDirectory, '..', '..');
const assetsDirectory = join(repositoryRoot, 'assets');
const commandIconsDirectory = join(assetsDirectory, 'icons');
const appIconSource = join(assetsDirectory, 'AppIcon.svg');
const installerBannerSource = join(assetsDirectory, 'InstallerBanner.svg');
const iconSizes = [16, 20, 24, 32, 48, 64, 128, 256];
const commandIconNames = [
    'git-bash',
    'git-gui',
    'git',
    'status',
    'pull',
    'fetch',
    'push',
    'commit',
    'log',
    'branch',
    'stash',
    'add',
    'diff',
    'blame',
    'restore',
    'clone',
    'init',
    'settings',
];

await mkdir(assetsDirectory, { recursive: true });
await mkdir(commandIconsDirectory, { recursive: true });

async function renderPng(source, destination, width, height = width) {
    await sharp(source, { density: 384 })
        .resize(width, height, { fit: 'contain' })
        .png({ compressionLevel: 9, adaptiveFiltering: true })
        .toFile(destination);
}

for (const size of iconSizes) {
    await renderPng(appIconSource, join(assetsDirectory, `AppIcon-${size}.png`), size);
}

await renderPng(appIconSource, join(assetsDirectory, 'AppIcon.png'), 256);
await renderPng(appIconSource, join(assetsDirectory, 'InstallerLogo.png'), 512);
await renderPng(installerBannerSource, join(assetsDirectory, 'InstallerBanner.png'), 1024, 320);
await renderPng(appIconSource, join(assetsDirectory, 'Square44x44Logo.png'), 44);
await renderPng(appIconSource, join(assetsDirectory, 'Square150x150Logo.png'), 150);
await renderPng(appIconSource, join(assetsDirectory, 'StoreLogo.png'), 50);

async function writeIco(source, destination) {
    const sourceBuffer = await readFile(source);
    const icoFrames = [];
    for (const size of iconSizes) {
        const png = await sharp(sourceBuffer, { density: 384 })
            .resize(size, size, { fit: 'contain' })
            .png({ compressionLevel: 9, adaptiveFiltering: true })
            .toBuffer();
        icoFrames.push({ size, png });
    }

    const headerSize = 6;
    const directoryEntrySize = 16;
    let imageOffset = headerSize + directoryEntrySize * icoFrames.length;
    const icoHeader = Buffer.alloc(headerSize);
    icoHeader.writeUInt16LE(0, 0);
    icoHeader.writeUInt16LE(1, 2);
    icoHeader.writeUInt16LE(icoFrames.length, 4);

    const directoryEntries = icoFrames.map(({ size, png }) => {
        const entry = Buffer.alloc(directoryEntrySize);
        entry.writeUInt8(size === 256 ? 0 : size, 0);
        entry.writeUInt8(size === 256 ? 0 : size, 1);
        entry.writeUInt8(0, 2);
        entry.writeUInt8(0, 3);
        entry.writeUInt16LE(1, 4);
        entry.writeUInt16LE(32, 6);
        entry.writeUInt32LE(png.length, 8);
        entry.writeUInt32LE(imageOffset, 12);
        imageOffset += png.length;
        return entry;
    });

    await writeFile(
        destination,
        Buffer.concat([icoHeader, ...directoryEntries, ...icoFrames.map(({ png }) => png)]),
    );
}

await writeIco(appIconSource, join(assetsDirectory, 'AppIcon.ico'));
for (const name of commandIconNames) {
    await writeIco(
        join(commandIconsDirectory, `${name}.svg`),
        join(commandIconsDirectory, `${name}.ico`),
    );
}

console.log(
    `Generated ${iconSizes.length + 7} brand assets and ${commandIconNames.length} command icons in ${assetsDirectory}.`,
);
