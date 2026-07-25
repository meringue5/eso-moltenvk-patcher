# ESO cursor mode and macOS focus precedents

- Date: 2026-07-25
- Scope: post-mortem context for Experiment 0010
- Local evidence remains authoritative; forum reports are supporting precedent

## Cursor-mode behavior

ESO's default `.` binding toggles cursor mode. Community reports on the
official ESO forum describe that mode as removing the normal reticle/crosshair
and requiring a return to normal camera mode for world targeting. A historical
UI bug report describes the client remaining in cursor mode after an interface
transition until the same binding is used or the client is restarted.

Sources:

- <https://forums.elderscrollsonline.com/en/discussion/603499/way-to-unlock-cursor-from-toon>
- <https://forums.elderscrollsonline.com/en/discussion/160571/patch-2-0-2-ui-bug-camera-getting-stuck-in-cursor-mode>
- <https://forums.elderscrollsonline.com/en/discussion/114370/opening-map-causes-the-game-to-go-into-mouse-freelook-mode>

This matches the Experiment 0010 user description but does not prove which UI
transition selected cursor mode in that run.

## macOS focus and frame rate

An older official Mac technical-support thread reports approximately 9 FPS
when ESO was not the active window and describes the mouse escaping the game.
Newer ESO discussion confirms that the client may deliberately limit background
FPS when it does not have focus.

Sources:

- <https://forums.elderscrollsonline.com/en/discussion/61840/full-screen-issues-using-a-second-monitor>
- <https://forums.elderscrollsonline.com/en/discussion/620741/client-focus>

Experiment 0010 does **not** match that cause: preserved WindowServer records
identify ESO as frontmost and deliver keyboard focus to its PID throughout the
world interval. The precedent explains why focus was checked; the local
evidence falsifies OS-level background throttling for this run.

## Apple-silicon display context

ZeniMax states that Apple-silicon Macs are not officially supported and lists
changing the macOS display resolution or using windowed mode as potential,
non-guaranteed workarounds:

- <https://help.elderscrollsonline.com/app/answers/detail/a_id/52307/>

The local M4 display was in a 1710 x 1107 logical mode backed by
3420 x 2214 pixels. ESO created a 3420 x 2146 final swapchain despite preserved
`FullscreenWidth=1920` and `FullscreenHeight=1200`. This local fact, not the
support article, establishes that the intended 1920 x 1200 effective extent
was not tested.
