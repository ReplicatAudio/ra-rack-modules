# ra-rec — 4-Track Recorder

A 4-track CV/audio recorder with individual per-track control plus global transport, all four tracks shown on a wide scope display. Each track has its own recording buffer (up to 8 minutes) and is saved to its own `.rarec` file.

## Display
A single wide scope readout shows all four tracks stacked as waveform lanes. Each lane always shows the **full recording**, stretched or shrunk to fit the width — so the waveform compresses as a recording grows. Lines are drawn in **purple**, and are always clamped inside the lane (CV that exceeds ±5V is trimmed at the lane edge rather than spilling out). Empty tracks show a flat line.

## Per-track controls (one column per lane)
Each of the four lanes has:
- **In n**: CV/audio input to record (and pass-through).
- **Rec n** button + **Tr g n** trigger: toggle recording for that track. The button glows red while recording.
- **Clr n** button + **Tr c n** trigger: clear that track's recording.

While a track is recording, its **Out n** monitors the live input; otherwise **Out n** passes the input through when stopped, or plays the recording back when the module is playing.

## Global transport
- **All Rec** button + **Tr g** trigger: start/stop recording on all four tracks at once.
- **All Clr** button + **Tr c** trigger: clear all four tracks.
- **Play** button + **Tr p** trigger: globally play/pause playback.
- **Reset** button + **Tr r** trigger: reset the playhead to the start on all tracks at once.

## Playback
Playback is global. When playing, each track loops independently at its own recorded length, so tracks of different lengths cycle on their own. Tracks are always looping.

## File saving
When a recording is started (any track, or all tracks) and no recording path has been chosen yet, ra-rec opens the system file browser to ask where to save and what the recording is called — the same pattern as the stock VCV recorder module.

There is one base name/path for all four recordings. Each track is written with a `_<n>` suffix:
- e.g. `mytrack.rarec` → `mytrack_0.rarec`, `mytrack_1.rarec`, `mytrack_2.rarec`, `mytrack_3.rarec`

The chosen path is remembered for the current take, so per-track records reuse it. Pressing **All Clr** starts a fresh take and will ask for a new name the next time recording starts.

## Notes
- Each track records to a `.rarec` binary file (see doc-dev/ra-rarec.md for the format).
- The base path is persisted; on patch load the four `_<n>` files are reloaded for review/playback.
- Recordings do not need to be the same length; each track is independent.
