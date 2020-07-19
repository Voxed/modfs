<h1 align="center">ModFS</h1>
<p align="center">
    <img width="150" src="icon.png">
</p>
ModFS is a command-line tool for non-destructively modding games on Linux.

## About
ModFS is a fuse filesystem which overlays multiple different mod directories over a main game directory shadowing already existing files. This allows you to replace game files without losing the original ones.

The tool also uses an overwrite directory to which new files created through the fuse filesystem will be saved. 

All of this is done case-insensetively in order to support a wide variety of mods made specifically for Windows.

## Building

First you need to install fuse3 and meson
### Arch linux
```
$ sudo pacman -S fuse3 meson
```
### Ubuntu (untested)
```
$ sudo apt install libfuse3-dev meson
```
then run the following commands
```
$ git clone https://github.com/Voxed/modfs.git
$ cd modfs
$ mkdir build
$ cd build
$ meson ..
$ ninja
$ chmod +x ModFS
```
and you should be able to run
```
./ModFS
```
now you can move the ModFS executable to wherever necessary or install it by moving it to the /usr/local/bin root directory.

## Configuration

ModFS is configured with json, you can set up a game like this

```json
{
    "game_directory": "<absolute path to game directory>",
    "overwrite_directory": "<absolute path to overwrite directory>",
    "mod_directory": "<absolute path to mod directory>",
    "mods": [
        "<name of mod with highest priority>",
        "<name of mod with next highest priority>",
        "..."
    ]
}
```
> Note: A mod will shadow all files from mods will lower priority and files from the game directory.

## Usage

Run ModFS using 
```
$ ModFS --cfg=<configuration file> <mountpoint> -f
```
> Note: The -f argument is not strictly required but will run the filesystem in the foreground so you don't need to manually unmount it.

Then you can just run the game executable in the mountpoint using wine.

**Steam games will often require you to set the SteamAppId environment variable to the app id of the game.**

## Example
Let's say we find ourselves with the following directories
```
mods
├── mod1
|   ├── notice.txt -> "I'm mod1!"
|   └── data
|       └── mod1.plugin
└── mod2
    ├── NOTICE.txt -> "I'm mod2!"
    └── Data
        └── mod2.plugin
game
├── game.exe
└── DATA
    └── game.plugin

overwrite
mountpoint
```
We can use the following config.json
```json
{
    "game_directory": "/game",
    "overwrite_directory": "/overwrite",
    "mod_directory": "/mods",
    "mods": [
        "mod1",
        "mod2"
    ]
}
```
Then run ModFS using
```
$ ModFS --cfg=config.json mountpoint -f
```
In order to generate the following hierarchy in our mountpoint
```
mountpoint
├── game.exe
├── notice.txt -> "I'm mod1!"
└── DATA
    ├── game.plugin
    ├── mod1.plugin
    └── mod2.plugin
```
