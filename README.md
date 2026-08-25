# anagram-plugin-example

## Prerequisites

- [Nix](https://nixos.org/download)

## Commands

Entering a shell sets `$BUILD_DIR`, so every command below works as-is in both.

### Enter the shell

```sh
nix develop            # native (BUILD_DIR=build)
nix develop .#anagram  # cross  (BUILD_DIR=build-anagram)
```

### Configure

```sh
cmake -S . -B "$BUILD_DIR" -G Ninja
```

### Build

```sh
cmake --build "$BUILD_DIR"
```

### Test

```sh
ctest --test-dir "$BUILD_DIR" --output-on-failure
```

| Test | Checks |
| - | - |
| `dsp` | The DSP unit tests. |
| `lv2_validate` | The bundle's Turtle files against the LV2 ontologies. |
| `lv2lint` | The built plugin's conformance, warnings included (`-E warn`). |

### Deploy (cross only)

```sh
scp -O -r "$BUILD_DIR"/bin/*.lv2 root@192.168.51.1:/root/.lv2/
ssh root@192.168.51.1 "systemctl restart jack2 lvgl-app"
```
