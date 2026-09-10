# ra-rec File Format

## Overview

ra-rec uses a custom binary file format for storing recorded audio data on disk. This format is simpler and more reliable than WAV for the module's internal use, avoiding the complexity of WAV header parsing and chunk alignment issues.

## File Extension

`.rarec`

## Binary Layout

```
[4 bytes]  Magic: "RAREC"
[4 bytes]  Sample rate (big-endian uint32)
[4 bytes]  Number of samples (big-endian uint32)
[N*4 bytes] Sample data (big-endian int32, scaled from float)
```

### Header

| Offset | Size | Description |
|--------|------|-------------|
| 0      | 4    | Magic bytes: `RAREC` (0x52 0x41 0x52 0x45 0x43) |
| 4      | 4    | Sample rate in Hz (big-endian uint32) |
| 8      | 4    | Number of samples (big-endian uint32) |

### Sample Data

Each sample is stored as a big-endian int32, scaled from the float range [-1.0, 1.0] to the int32 range [-2147483647, 2147483647].

```
int32_t val = static_cast<int32_t>(clamp(sample, -1.0f, 1.0f) * 2147483647.0f);
```

## Reading

To read a `.rarec` file:
1. Read the first 4 bytes and verify they match "RAREC"
2. Read the next 4 bytes as the sample rate (big-endian uint32)
3. Read the next 4 bytes as the number of samples (big-endian uint32)
4. Read `numSamples * 4` bytes as the sample data
5. Convert each int32 sample back to float by dividing by 2147483647.0

## Writing

To write a `.rarec` file:
1. Write the magic bytes "RAREC"
2. Write the sample rate as a big-endian uint32
3. Write the number of samples as a big-endian uint32
4. For each sample, clamp to [-1.0, 1.0], multiply by 2147483647.0, cast to int32, and write as big-endian

## File Storage

Recordings are stored in the Rack user data directory under `ra-recordings/`. The path is generated as:
```
{asset::user("ra-recordings")}/ra-rec-{module_pointer}-{channel}.rarec
```

File paths are persisted in the patch JSON via `dataToJson`/`dataFromJson`.

## Persistence

- Recordings are saved to disk when recording stops
- File paths are stored in the patch JSON
- Recordings survive patch save/load and Rack restarts
- Each channel gets its own `.rarec` file
