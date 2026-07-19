# Troubleshooting history

## Exit crash and settings not saving

Initial symptom: ESO could be played, but normal logout/exit crashed as control
returned to the launcher. The crash prevented settings and agreement state from
being saved reliably.

Tested observations:

- Gameplay itself was generally stable.
- The crash clustered around logout/exit and return to the launcher.
- Bluetooth audio was not required to reproduce it.
- General audio use was not a sufficient explanation.
- Adding ESO/launcher applications to macOS permissions did not eliminate it.
- Launching `eso.app` directly breaks the required Steam authentication path.
- The practical workaround was to return to character select, wait briefly,
  then exit. This reduced disruption but did not constitute a fix.

Experiment 0003 provided a concrete shutdown report after about 2 hours 27
minutes of otherwise usable gameplay on the original embedded runtime. It was
`EXC_BAD_INSTRUCTION / SIGILL` on an audio cleanup thread whose stack included
`ExtendedAudioBufferList_Destroy` and `AudioComponentInstanceDispose`. This
confirms that audio teardown is involved in at least that exit crash; it does
not prove an audio-device fault, and it is distinct from the MoltenVK bridge's
startup `RIP=0` failure.

## Performance degradation

Symptoms:

- Towns and other-player density reduce FPS more than static object density.
- FPS can remain in the high 50s and later settle near the mid 30s without an
  obvious scene or combat change.
- Leaving the current world through logout and logging back in restores FPS.

Checks:

- Metal HUD showed `Thermal: Nominal` in both healthy and degraded captures.
- The degraded capture showed a large GPU-time increase and approximately 1 GB
  more app memory.
- Water/reflection options can add cost but do not explain logout recovery by
  themselves.

## Diagnostic discipline

For future tests, record the same camera position and wait 20 seconds before
capturing. Include FPS, GPU time, frame interval, app memory, Metal memory,
thermal state, player count, zone, and minutes since login. Do not compare only
the FPS number from different scenes.
