# Dfps

Dynamic screen refresh rate controller for Android. Splited from [Uperf v2](https://github.com/yc9559/uperf).

## Feature

- Dynamically switch screen refresh rate on touch and gesture event
- Per-app configuration
- Two low-level methods: `PEAK_REFRESH_RATE` and `Surfaceflinger backdoor`
- Stop dynamically switch screen refresh rate when the brightness is low
- Support Android 10-13

## Usage

Read [dfps_help_en.md](magisk/config/dfps_help_en.md) for help.

## Download

[Releases](https://github.com/yc9559/dfps/releases)

## License

Dfps is licensed by [Apache License, Version 2.0](https://github.com/yc9559/dfps/blob/master/LICENSE).

Dfps uses the following third-party source code or libraries:

1.	[gabime/spdlog version 1.9.2](https://github.com/gabime/spdlog)
2.	[eliaskosunen/scnlib version 0.4.0](https://github.com/eliaskosunen/scnlib)
