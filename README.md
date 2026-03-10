<div align="center">
  <img height="150" src="https://raw.githubusercontent.com/neoapps-dev/legacy-launcher/refs/heads/main/packaging/icon.png">
  <h1>Legacy Launcher</h1>
  <p>Launcher for Minecraft Legacy Console Edition on GNU/Linux</p>
</div>

<img width="1340" height="837" alt="Screenshot" src="https://github.com/user-attachments/assets/1a425748-ae9f-46a9-b12b-3f4ff5cfa8bc" />

## Important!
This project does ***not*** include the source code for Minecraft Legacy Edition, nor any code copyrighted by Mojang AB, Microsoft, or 4J Studios.

## UPDATE!!
LegacyLauncher 1.2.0 introduces [WeaveLoader](https://github.com/Jacobwasbeast/LegacyWeaveLoader) support for mods!

## Supported Platforms
Legacy Launcher is designed primarily for GNU/Linux. Windows or macOS support is not on our list, nor is currently supported.
Though contributions to add support for those platforms are very welcome.

## Installation
| Type | Link |
| --- | ----------- |
| Native Build | [here](https://github.com/neoapps-dev/legacy-launcher/releases/latest/download/legacy-launcher) |
| AppImage (x86_64) | [here](https://github.com/neoapps-dev/legacy-launcher/releases/latest/download/LegacyLauncher-1.3.0-x86_64.AppImage) |
| Flatpak | [here](https://github.com/neoapps-dev/legacy-launcher/releases/latest/download/LegacyLauncher-1.3.0.flatpak) |

## Building from source
The source code for Legacy Launcher uses the CMake build system. A Makefile is also provided for ease of use.
```sh
make                  # compile native
make install          # compile native and install it
make debug            # compile debug rather than release
make flatpak          # compile flatpak
make flatpak-install  # compile flatpak and install it
make flatpak-run      # run the installed flatpak
make appimage         # compiles an AppImage
make clean            # cleans the directory from build output
```

## Extras
Legacy Launcher also comes with a few flags you can use:
- `--auto`: Runs the last instance launched without showing the launcher window

## License
Legacy Launcher is licensed under the [MIT License](LICENSE)
