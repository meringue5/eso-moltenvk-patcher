# teso4m4

Experimental macOS performance research and runtime tooling for the Steam build
of **The Elder Scrolls Online** on Apple Silicon.

> [!WARNING]
> The MoltenVK bridge is a research prototype. The first full redirection test
> successfully loaded MoltenVK 1.4.1 and redirected Vulkan entry points, but ESO
> crashed afterward. It is **not ready for normal gameplay**.

`teso4m4` documents reproducible findings, conservative graphics settings, and
an experimental method for redirecting ESO's statically linked MoltenVK 1.0.18
runtime to a current dynamic MoltenVK release while preserving Steam launch and
authentication.

## Current status

- Confirmed ESO is an x86_64 app running through Rosetta on Apple Silicon.
- Confirmed ESO statically links MoltenVK 1.0.18; replacing the bundled
  framework/archive alone has no effect.
- Confirmed official MoltenVK 1.4.1 supports ESO's Vulkan 1.0 extension set and
  creates a device on Apple M4.
- Built a Bink re-export proxy that loads before ESO and can patch executable
  entry points under Rosetta.
- First 17-entry-point redirection test became active, then ESO crashed.
- Original game loader and old pipeline cache were restored successfully.

See [Findings](docs/FINDINGS.md), [Experiment 0001](docs/experiments/0001-moltenvk-1.4.1-full-redirect.md),
the [Crash analysis](docs/CRASH_ANALYSIS.md), and the [Roadmap](docs/ROADMAP.md).

## Repository layout

```text
config/       Build fingerprints and non-personal settings snapshots
docs/         Findings, settings guidance, troubleshooting, experiments
scripts/      Fetch, build, install, restore, and status helpers
src/          Runtime bridge source
tools/        Binary-analysis and compatibility probes
```

## Build overview

Requirements: macOS, Xcode command-line tools, Python 3, and a locally installed
Steam copy of ESO. No ESO or Bink binaries are distributed by this repository.

```sh
./scripts/fetch-moltenvk.sh
./scripts/build.sh
```

Installation is deliberately gated because the current bridge crashes:

```sh
TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK ./scripts/install.sh
./scripts/restore.sh
```

## Scope and legal note

This project is unaffiliated with ZeniMax, Bethesda, Valve, Apple, or Khronos.
It does not distribute proprietary game files, credentials, or caches. MoltenVK
is fetched from its official release and remains under its own Apache 2.0
license. Use this project only with software and accounts you are authorized to
operate.

## 한국어 요약

Steam판 ESO의 macOS 성능 저하와 종료 크래시를 조사하고, 오래된 정적
MoltenVK를 최신 런타임으로 우회하는 실험 프로젝트입니다. 현재 설정 및
분석 자료는 유효하지만 MoltenVK 브리지는 아직 플레이용이 아닙니다.
