# Troubleshooting history

## First starts after installation

On ESO 12.0.8, the user has repeatedly observed a start that shows the pink
startup surface and then runs at approximately 10 FPS, followed by a clean,
smooth restart. Experiment 0035 captured one exact pair in which restarting ESO
alone after four seconds was sufficient; the launcher was not restarted.

Both processes loaded MoltenVK 1.4.2, activated all 17 redirects, and reached
the same compositor latch, so pink plus low FPS does not by itself mean the
bridge was inactive. The bad process recorded no normal ESO-side connection to
`MTLCompilerService` during its approximately 42-second lifetime. The smooth
restart recorded ten connection events and six successful Metal compilation
jobs. Failure to enter the normal pipeline-compilation path is now the leading
hypothesis, but cause and consequence are not yet separated.

If the condition occurs, quitting ESO and starting it again through the same
normal launcher path is the currently observed recovery; restarting the
launcher is not always necessary. Preserve all caches. Do not delete them or
distribute a warmed cache as a workaround. Future diagnosis should capture
cache metadata, process boundaries, fixed-scene FPS/GPU timing, memory, thermal
state, and compiler-service engagement for each start.

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
