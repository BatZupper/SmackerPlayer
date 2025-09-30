# Smacker Player

a simple player for the Smacker (.smk) file format written in C

## Why use it

It can be useful if you need a quick and lightweight way to play and extract Smacker files but it's not really raccomanded since VLC and FFMPEG can already do it (this program was done before I knew it) so it can also be a nice way to learn more about the format and the libraries used other than a nice little experiment and project for myself

## How to use it

It's quite simple you just need to run in the terminal

```
SmackerPlayer (name of the file).smk
```

or set the player as the default app for .smk files and then you can play your video with no issues

if you want to export it after opening the player with the file just press E and it will convert the file as a .AVI

To quit the player just use Q or ESC (escape)

## Libraries used

Rendering/Event Manager: [SDL3](https://github.com/libsdl-org/SDL)

Audio: [Miniaudio](https://github.com/mackron/miniaudio)

Reading the Smacker file: [Libsmacker](https://github.com/JonnyH/libsmacker)

## How to build

You just need GCC and the SDL3 DLL and by running the compile.bat script you're good to go
