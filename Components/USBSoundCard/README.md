# USBSoundCard Component

## Overview

The USBSoundCard component provides audio capture and transmission capabilities for AMSat applications. It can capture audio from a USB sound card and transmit the audio data as FPrime packets.

## Features

### Audio Capture
- Real-time audio capture from USB sound card devices
- Support for multiple ALSA device formats
- Configurable sample rate (44.1kHz default)
- Mono channel support
- Audio level monitoring and telemetry

### Audio Transmission
- **NEW**: FPrime packet transmission functionality
- Real-time audio data streaming
- Packet sequence numbering
- Timestamped packets
- Configurable packet sizes
- Test packet generation

## Commands

### Audio Capture Commands
- `START_CAPTURE` (opcode 0): Start audio capture from USB sound card
- `STOP_CAPTURE` (opcode 1): Stop audio capture

### Transmission Commands
- `START_TRANSMISSION` (opcode 2): Start audio transmission as FPrime packets
- `STOP_TRANSMISSION` (opcode 3): Stop audio transmission
- `SEND_TEST_PACKET` (opcode 4): Send a test packet to verify transmission functionality

## Telemetry

### Audio Health
- `AUDIO_INPUT_LEVEL`: Current audio input level (0-255)
- `DEVICE_CONNECTED`: Audio device connection status
- `FRAMES_PROCESSED`: Number of audio frames processed
- `AUDIO_PEAK_LEVEL`: Peak audio level in last second

### Transmission Status
- `TRANSMISSION_ACTIVE`: Transmission status
- `PACKETS_TRANSMITTED`: Number of packets transmitted
- `LAST_TRANSMISSION_TIME`: Last transmission timestamp

## Events

### Audio Events
- `AUDIO_CAPTURE_STARTED`: Audio capture started
- `AUDIO_CAPTURE_STOPPED`: Audio capture stopped
- `DEVICE_DISCONNECTED`: USB sound card disconnected
- `AUDIO_CAPTURE_ALREADY_STARTED`: Audio capture already started
- `AUDIO_LEVEL_HIGH`: Audio input level too high

### Transmission Events
- `AUDIO_TRANSMISSION_STARTED`: Audio transmission started
- `AUDIO_TRANSMISSION_STOPPED`: Audio transmission stopped
- `AUDIO_TRANSMISSION_ALREADY_STARTED`: Audio transmission already started
- `PACKET_TRANSMITTED`: Audio packet transmitted successfully
- `TRANSMISSION_ERROR`: Transmission error occurred

## Ports

### Input Ports
- `run`: Scheduled input port for periodic audio processing

### Output Ports
- `packetOut`: Port for sending FPrime packets
- `bufferOut`: Port for sending buffer packets

## FPrime Packet Format

The component transmits audio data in the following FPrime packet format:

```
[Header - 12 bytes]
- Sequence Number (U32): Incremental packet sequence
- Timestamp (U32): FPrime timestamp in seconds
- Audio Data Size (U32): Size of audio data in bytes

[Audio Data - Variable size]
- Raw audio samples (16-bit signed PCM)
```

### Packet Structure
```
Offset  | Size | Field           | Description
--------|------|-----------------|------------------
0       | 4    | Sequence        | Packet sequence number
4       | 4    | Timestamp       | FPrime timestamp (seconds)
8       | 4    | Audio Data Size | Size of audio data
12      | N    | Audio Data      | Raw audio samples
```

## Usage Example

### Basic Audio Capture
```cpp
// Start audio capture
component.START_CAPTURE_cmdHandler(0, 1);

// Process audio data (called periodically)
component.run_handler(0, 0);

// Stop audio capture
component.STOP_CAPTURE_cmdHandler(1, 2);
```

### Audio Transmission
```cpp
// Start transmission
component.START_TRANSMISSION_cmdHandler(2, 1);

// Start audio capture (transmission will begin automatically)
component.START_CAPTURE_cmdHandler(0, 2);

// Send test packet
component.SEND_TEST_PACKET_cmdHandler(4, 3);

// Stop transmission
component.STOP_TRANSMISSION_cmdHandler(3, 4);

// Stop audio capture
component.STOP_CAPTURE_cmdHandler(1, 5);
```

## Testing

### Running Tests
```bash
# Run all tests
cd Components/USBSoundCard/test
./run_tests.sh

# Or run individually
fprime-util build --ut
fprime-util check --ut
```

### Test Coverage
- **Unit Tests**: Basic component functionality, command handling, error conditions
- **Integration Tests**: Packet transmission, FPrime packet format validation
- **Packet Validation**: Sequence numbering, timestamp validation, audio data integrity

### Test Components
- `USBSoundCardTester`: Main unit test harness
- `PacketReceiver`: Packet validation and statistics
- `IntegrationTest`: End-to-end transmission testing

## Hardware Requirements

- USB sound card device
- ALSA-compatible audio system
- Linux kernel with ALSA support

## Dependencies

- ALSA library (`libasound2-dev`)
- FPrime framework
- C++11 or later

## Build Configuration

The component automatically handles ALSA library linking:
- Native builds: Links against ALSA library
- Cross-compilation: Stubs out ALSA functionality for deployment

## Troubleshooting

### Common Issues
1. **No audio device found**: Check USB sound card connection and ALSA device names
2. **Permission denied**: Ensure user has access to audio devices
3. **Transmission errors**: Verify output ports are connected to receivers

### Debug Output
The component provides debug output with `[USB_SOUND]` prefix:
- Device connection attempts
- Audio level monitoring
- Transmission status
- Error conditions

## Future Enhancements

- Multi-channel audio support
- Configurable sample rates
- Audio compression
- Network transmission protocols
- Audio playback capabilities 