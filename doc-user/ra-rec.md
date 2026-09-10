# ra-rec — 4-Channel CV/Audio Recorder

Records and plays back CV/audio signals on 4 independent channels. Each channel has its own input, output, and recording buffer (up to 8 minutes per track). Recordings are saved to `.rarec` binary files on disk and persist across patch saves and Rack sessions.

## Controls
- **Rec**: toggle record on/off. The button glows red while recording. Also triggered by the Record CV input.
- **Play**: toggle playback on/off. The button glows purple while playing. Also triggered by the Play CV input.
- **Rate**: playback rate, 0×–4×. CV input attenuates the knob value.
- **Position**: scrubs the playhead position (0–100%). When playback is active, the playhead continues from the new position.

## Inputs
- **In 1–4**: CV/audio signals to record. Passed through to outputs when not recording or playing.
- **Rec tr**: external trigger to toggle recording.
- **Play tr**: external trigger to toggle playback.
- **Rate CV**: CV to modulate playback rate.
- **Position CV**: CV to scrub the playhead position.

## Outputs
- **Out 1–4**: Recorded/played-back CV/audio signals. When not recording or playing, inputs pass through to outputs.

## Notes
- Each channel's recording is saved as a `.rarec` binary file in the Rack user data directory (`ra-recordings/`).
- Recordings persist across patch saves, Rack restarts, and session restores.
- When recording starts, all channels' buffers are cleared.
- When playback starts, the playhead resets to the beginning of the recording.
- The position knob/CV works as a scrubber when not playing, and sets the playhead position during playback (playback continues from the new position).
- Playback loops when it reaches the end of the recording.
