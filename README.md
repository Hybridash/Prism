# Hybrid Launcher

Hybrid Launcher is a Minecraft launcher based on [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher). It adds a few features to help when you play with mods.

> **Hybrid Launcher is not Prism Launcher.** It is an unofficial fork and is not endorsed by or affiliated with the Prism Launcher project (https://prismlauncher.org). Please report problems with Hybrid Launcher [here](https://github.com/Hybridash/Prism/issues), not to the Prism Launcher team.

## What's added

Each feature can be turned on or off in **Settings → Hybrid Extras**.

| Feature | What it does | Default |
|---|---|---|
| **World backups** | Before each launch, any world that changed since its last backup is zipped into `<instance>/backups/<world>/`. The newest 5 backups of each world are kept. On the **Worlds** page, **Back Up Now** makes a backup right away, and **Restore Backup...** rolls a world back. The current version of the world is kept as a copy, never deleted. | On |
| **Crash explainer** | When the game crashes, a short "What went wrong?" section is added to the end of the log. It covers common causes: missing or wrong-version mods, the wrong Java version, running out of memory, broken mixins, graphics driver problems, and the mod a crash report points to. Each cause comes with a fix. | On |
| **Mod checker** | Before launch, it warns about the same mod installed twice, mods known to break each other (for example OptiFine + Sodium, or Sodium + Embeddium), and required mods that seem to be missing. It only warns and never stops the game from starting. | On |
| **RAM advice** | Suggests a maximum memory setting based on how many mods you have and how much RAM your computer has. | On |
| **Shared keybinds and servers** | Uses the same keybinds and multiplayer server list in every instance. When a game closes, they are saved, and the next instance you launch gets them. Video and sound settings stay separate for each instance. | Off |

Everything else, including playtime tracking per instance, works the same as in Prism Launcher.

## Downloads

Every push to `main` is built automatically for Linux, Windows and macOS. Open the [Actions tab](https://github.com/Hybridash/Prism/actions), click the latest successful **Build** run, and download the file for your system from **Artifacts** at the bottom. You need to be signed in to GitHub to download them.

These are test builds. They are not signed, so Windows and macOS may warn you the first time you open them.

## Building it yourself

It builds the same way as Prism Launcher. Follow the [Prism Launcher build instructions](https://prismlauncher.org/wiki/development/build-instructions), using this repository instead.

The new code is in:

- `launcher/hybrid/`: crash explainer and world backup logic
- `launcher/minecraft/launch/`: `BackupWorlds`, `CheckModConflicts`, `RecommendMemory`, `SyncSharedConfig` launch steps
- `launcher/ui/pages/global/HybridPage.*`: the settings page
- `tests/CrashExplainer_test.cpp`, `tests/WorldBackup_test.cpp`: tests

## Notes for sharing this fork

- The auto-updater is turned off, so Hybrid Launcher never "updates" itself into official Prism Launcher.
- The build still uses Prism Launcher's Microsoft login ID and CurseForge API key (see the top-level `CMakeLists.txt`). Prism asks forks to use their own. That's fine for personal use, but replace them before you publish builds for other people.
- Hybrid Launcher stores its data in the same folder as Prism Launcher, so if you have both installed they share instances and settings.

## License

Like Prism Launcher, all launcher code is available under the GPL-3.0-only license. Credit for the launcher goes to the Prism Launcher, PolyMC and MultiMC contributors.
