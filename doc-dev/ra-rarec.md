# ra-rec File Format

## Overview

ra-rec uses standard 16-bit mono PCM WAV files for storing recorded audio on disk. This replaces the earlier custom `.rarec` binary format.

## File Extension

`.wav`

## WAV Layout

Each track is saved as a standard RIFF/WAVE file:

| Offset | Size | Description |
|--------|------|-------------|
| 0      | 4    | Magic bytes: `RIFF` |
| 4      | 4    | File size minus 8 (little-endian uint32) |
| 8      | 4    | Format: `WAVE` |
| 12     | 4    | Subchunk ID: `fmt ` |
| 16     | 4    | Subchunk size: 16 (little-endian uint32) |
| 20     | 2    | Audio format: 1 (PCM, little-endian uint16) |
| 22     | 2    | Number of channels: 1 (mono, little-endian uint16) |
| 24     | 4    | Sample rate in Hz (little-endian uint32) |
| 28     | 4    | Byte rate (little-endian uint32) |
| 32     | 2    | Block align: 2 (little-endian uint16) |
| 34     | 2    | Bits per sample: 16 (little-endian uint16) |
| 36     | 4    | Subchunk ID: `data` |
| 40     | 4    | Data size in bytes (little-endian uint32) |
| 44     | N    | Sample data (little-endian int16) |

### Sample Data

Each sample is stored as a little-endian int16, scaled from the float range [-1.0, 1.0]:

```
int16_t val = static_cast<int16_t>(clamp(sample, -1.0f, 1.0f) * 32767.0f);
```

## File Storage

When a recording starts and no path has been chosen, the module opens the system file browser to ask where to save the recording. One base name/path is shared by all four tracks, each written with a `_<n>` suffix inserted before the extension:
```
{user-chosen base}.wav  →  {base}_0.wav, {base}_1.wav, {base}_2.wav, {base}_3.wav
```

The base path is persisted in the patch JSON via `dataToJson`/`dataFromJson`, and on load the four `_<n>` files are read back into the track buffers.

## Sample Rate Conversion

When loading a WAV file with a different sample rate than the engine, linear interpolation is used to resample the audio to the engine's sample rate.
