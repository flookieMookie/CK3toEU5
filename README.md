# CK3 to EU5 Converter

Turn your Crusader Kings III save into a Europa Universalis V mod and keep playing your world in 1337.

**[Download the latest version](https://github.com/flookieMookie/CK3toEU5/releases/latest)** · Windows, 64-bit · Crusader Kings III + Europa Universalis V 1.3

## How to use it

1. Download the zip from the [releases page](https://github.com/flookieMookie/CK3toEU5/releases) and unzip it anywhere.
2. Run **CK3toEU5Launcher.exe**. If Windows warns you the program isn't signed, click **More info**, then **Run anyway**.
3. Pick your CK3 save. They're in `Documents\Paradox Interactive\Crusader Kings III\save games`.
4. Check the game folders say **Found**, then press **Convert**. The mod goes straight into EU5's mod folder.
5. In EU5, click the shield icon on the main menu, enable your mod, and start a new game.

The launcher tells you when a new version is out.

EU5 always starts on 1 April 1337, so a campaign played to around then fits best, but a save from any date works.

## What carries over

- **Your map:** every realm becomes an EU5 country with its borders, capital, name, rank and flag. Big realms stay in one piece; only vassal kings become subjects
- **Diplomacy:** wars in progress (with levies raised), alliances, truces and tributaries
- **Characters:** rulers with their spouse, children, heir, dynasty and council, with abilities, traits and nicknames from CK3
- **Your realm:** treasury, succession law, centralisation from crown or tribal authority, and men-at-arms as a standing army
- **The land:** religion and culture of every area (including cultures and faiths your campaign created), development, and the castles, forts and market villages you built
- **Technology** from your development and your culture's era
- **Names in every language** EU5 supports
- **CK3 mods** your save used, if you have them installed (map-changing mods can't be converted)

Places CK3 doesn't cover, like the Americas, keep EU5's normal 1337 setup. Population sizes stay EU5's own.

## Problems?

Open an [issue](https://github.com/flookieMookie/CK3toEU5/issues) with your save's CK3 version and the `log.txt` from the converter's folder.

## About

Built on the [Paradox Game Converters](https://github.com/ParadoxGameConverters/CK3toEU5) CK3toEU5 project, developed further with the help of AI. It isn't made or supported by that team or by Paradox Interactive, so please report problems here, not to them.

MIT licensed. License texts for everything included are in the download's `licenses` folder.

## Building from source
This project uses CMake and can be build using any modern C++ toolchain that supports C++23, though Visual Studio is the most used and tested.

To start, first clone the repository. Then open a terminal window to the location where it was cloned and run the command `git submodule update --init --recursive`. When this has finished, open the folder in your IDE of choice.

There are three CMake configurations: x64 release, x64 debug, and x64 clang-tidy. The last runs a series of static analysis checks on the code base.

The Windows build also produces `CK3toEU5Launcher.exe`, the launcher the releases ship. Work happens on the `preview` branch.

## Checking a conversion
`python tools/validate_output.py <converted mod folder> [<EU5 install folder>]` checks a converted mod against EU5's own definitions: valid UTF-8 and balanced braces in every file, every country, character and dynasty referred to is defined, every localisation key exists in every language, and every ruler trait passes EU5's allow rules. It exits non-zero if it finds a problem.
