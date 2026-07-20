# teso4m4

Experimental macOS performance research and runtime tooling for the Steam build
of **The Elder Scrolls Online** on Apple Silicon.

> [!WARNING]
> The MoltenVK bridge is a research prototype. Experiment 0004 loaded MoltenVK
> 1.4.1, successfully hid the new HDR device extension, and still crashed at
> the confirmed NULL HDR-metadata call. That failed state was preserved until
> restoration became necessary for the next clean rebuild. The Experiment 0005
> surface-format filter then prevented that call and completed its approved
> character-selection test without a crash, but produced severe transient pink
> output and persistent black-layer/shadow flicker. The installed Experiment
> 0006 candidate disabled Metal argument buffers, removed that flicker, and
> reached the world. Enabling SSAO during play then reduced output to changing
> solid colors without crashing. That exact settings state is preserved, and
> the active file has been returned to the verified SSAO-disabled baseline.
> Opening the launcher before the planned repeat installed ESO 12.0.7 and
> changed the executable fingerprint. The bridge has now been statically
> rebased, rebuilt, validated without the game, and reinstalled for that exact
> target. A subsequent launch passed the automatic bridge-startup checks and
> exited without a crash, but server maintenance prevented character selection
> or world entry. It remains ready only for the controlled five-minute repeat
> in the [current status](docs/STATUS.md) after service returns.

`teso4m4` documents reproducible findings, conservative graphics settings, and
an experimental method for redirecting ESO's statically linked MoltenVK 1.0.18
runtime to a current dynamic MoltenVK release while preserving Steam launch and
authentication.

## Current status

The latest verified baseline, active blocker, and next test gate are maintained
in [Project status](docs/STATUS.md). Use that dated snapshot rather than this
landing page when deciding whether an experiment is safe to continue.

Start with the [documentation guide](docs/README.md), then use the
[experiment index](docs/experiments/README.md) for historical evidence.

## Repository layout

```text
config/             Build fingerprints and non-personal settings snapshots
docs/               Current status, findings, plans, and technical guidance
docs/experiments/   Append-only experiment records and their evidence summaries
docs/research/      Dated external precedent reviews and literature notes
scripts/            Fetch, build, install, restore, and status helpers
src/                Runtime bridge source
tools/              Binary-analysis and compatibility probes
```

## Build overview

Requirements: macOS, Xcode command-line tools, Python 3, and a locally installed
Steam copy of ESO. No ESO or Bink binaries are distributed by this repository.

```sh
./scripts/fetch-moltenvk.sh
./scripts/build.sh
```

Installation is deliberately gated because live bridge builds have crashed or
produced severe rendering corruption during graphics startup:

```sh
TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK ./scripts/install.sh
./scripts/restore.sh
```

## Update gate

Before preparing an experiment, check whether the launcher has replaced the
local ESO executable:

```sh
./scripts/check-update.sh
```

Exit status `0` means the exact selected target is still present. An unknown or
historical build exits `3`. If a new build retains the verified binary layout,
`scripts/rebase-update.sh` can audit and create a new target manifest without
modifying the game bundle. See the [update runbook](docs/UPDATES.md); no target
is installed merely because the fast static audit passes.

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
