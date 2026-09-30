# ra-rec — 4-Track Recorder

A 4-track CV/audio recorder with individual per-track control plus global transport, all four tracks shown on a wide scope display. Each track has its own recording buffer (up to 8 minutes) and is saved to its own `.wav` file.

## Display
A single wide scope readout shows all four tracks stacked as waveform lanes. Each lane always shows the **full recording**, stretched or shrunk to fit the width — so the waveform compresses as a recording grows. Lines are drawn in **purple**, and are always clamped inside the lane (CV that exceeds ±5V is trimmed at the lane edge rather than spilling out). Empty tracks show a flat line. A playhead marker shows the current playback position — solid white while playing, flashing when paused.

## Per-track controls (one column per lane)
Each of the four lanes has:
- **In n**: CV/audio input to record (and pass-through).
- **Rec n** button + **Tr g n** trigger: toggle recording for that track. The button glows red while recording.
- **Clr n** button + **Tr c n** trigger: clear that track's recording.
- **▶ n** button + **Tr p n** trigger: toggle playback for that track. The button glows purple while playing.
- **Rst n** button + **Tr r n** trigger: reset the playhead to the start.
- **◀ n** button + **Tr v n** trigger: toggle reverse playback direction. The button glows cyan when reversed.
- **Spd n** knob + **CV s n** input: playback speed. The knob sets a base speed of 0–8× (default 1×). The CV input attenuates the speed: 0 V = no change, 10 V = double the base speed.
- **Pos n** input: scrub/position control. 0 V = start, 10 V = end. When not playing, the output follows the scrubbed position for audible scrubbing.
- **Wr n** button: save the track's recording to a user-chosen `.wav` file.
- **Rd n** button: load a `.wav` file into the track.
- **Out n**: output. While recording, monitors the live input. While playing, outputs the recorded audio. When stopped, passes the input through.

## Global transport
- **All Rec** button + **Tr g** trigger: start/stop recording on all four tracks at once.
- **All Clr** button + **Tr c** trigger: clear all four tracks.
- **▶ All** button + **Tr p** trigger: toggle playback on all tracks.
- **Rst All** button + **Tr r** trigger: reset the playhead to the start on all tracks.

## Playback
Each track loops independently at its own recorded length, so tracks of different lengths cycle on their own. Playback speed is per-track and can be reversed. The **Pos n** input allows scrubbing to any position in the recording.

## File saving
When a recording is started (any track, or all tracks) and no recording path has been chosen yet, ra-rec opens the system file browser to ask where to save and what the recording is called — the same pattern as the stock VCV recorder module.

There is one base name/path for all four recordings. Each track is written with a `_<n>` suffix:
- e.g. `mytrack.wav` → `mytrack_0.wav`, `mytrack_1.wav`, `mytrack_2.wav`, `mytrack_3.wav`

The chosen path is remembered for the current take, so per-track records reuse it. Pressing **All Clr** starts a fresh take and will ask for a new name the next time recording starts.

Recordings are saved as 16-bit mono WAV files. The **Wr n** and **Rd n** buttons allow manual export/import of individual tracks to/from any `.wav` file.

## Notes
- Each track records to a `.wav` file (16-bit mono PCM).
- The base path is persisted; on patch load the four `_<n>` files are reloaded for review/playback.
- Recordings do not need to be the same length; each track is independent.
- Sample rate conversion is performed when loading WAV files recorded at a different sample rate.
