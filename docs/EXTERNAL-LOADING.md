# Future update-independent bridge loading

Direction clarified by the user on 2026-11-06. The intended architecture is
preserved external bridge code, a reliable per-launch loading route that
survives official package updates, and independent discovery of compatible
connection points in the updated game. With all three established, manual
reinstallation could become unnecessary. Separating the bridge file alone does
not establish that its loading route survives an updater.

Stable 0.3.0 supplies dynamic discovery with manual installation as the release
baseline. Update-independent loading remains unproven and is not shipped.
Automatic reinstallation is a fallback approach, not a substitute for this
design objective. No promise of universal future code compatibility is made.
