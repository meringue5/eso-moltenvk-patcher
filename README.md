# teso4m4

Experimental macOS performance research and runtime tooling for the Steam build
of **The Elder Scrolls Online** on Apple Silicon.

> [!WARNING]
> The bridge remains an exact-build research prototype, not a generally
> supported game patch. The current Apple M4 checkpoint uses official MoltenVK
> 1.4.2 and has passed extended ordinary play at relatively high settings,
> including user-controlled resolution and graphics resets that previously
> corrupted output on 1.4.1. A roughly one-second full-screen hot-pink frame
> still appears during startup, before normal UI rendering, but has no observed
> gameplay impact. See the [current status](docs/STATUS.md) and the
> [sanitized standard settings](config/usersettings-m4-moltenvk-1.4.2-standard.txt).

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
local ESO executable, whether its completed remote comparison is current, and
whether the installed bridge checkpoint still matches:

```sh
./scripts/quick-update-check.sh
```

`READY` is the routine fast path. It covers the executable identity, client
version/databuild, launcher repository state, and installed bridge checkpoint.
`STOP` exits `3` and exposes the failing component for deeper analysis. The
component checks remain available individually:

```sh
./scripts/check-update.sh
./scripts/check-launcher-state.sh
```

These passive checks do not start an app or contact the network. If a new ESO
build retains the verified binary layout,
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
MoltenVK를 최신 런타임으로 우회하는 실험 프로젝트입니다. 현재 Apple M4
체크포인트는 공식 MoltenVK 1.4.2와 비교적 높은 그래픽 설정으로 장시간
플레이 및 설정·해상도 변경을 통과했습니다. 시작 직후 약 1초간 전체 화면이
핫핑크로 보이는 글리치는 남아 있습니다.
