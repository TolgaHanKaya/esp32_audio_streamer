# ESP32-S3 Real-Time Wi-Fi Audio Streamer

## Overview
This Proof of Concept (PoC) demonstrates a stable, low-latency real-time audio streaming architecture using an ESP32-S3 microcontroller and an INMP441 MEMS microphone. It captures raw acoustic data via the hardware I2S (Inter-IC Sound) protocol, processes it on the fly, and serves a continuous 16-bit PCM WAV audio stream over a local Wi-Fi network directly to any standard web browser. 

By bypassing high-level software wrappers in favor of native ESP-IDF silicon controls, this architecture successfully eliminates compounding network latency and pitch distortion, establishing a highly reliable Minimum Viable Product (MVP) baseline for future IoT acoustics, OLED visualization, and Digital Signal Processing (DSP) integrations.

## Tech Stack
- **Hardware:** ESP32-S3 DevKitC
- **Sensor:** INMP441 Omnidirectional I2S Microphone
- **Software Framework:** Arduino Core for ESP32 (utilizing native ESP-IDF `driver/i2s.h`)
- **Network Protocol:** HTTP over TCP/IP (Wi-Fi Station Mode)
- **Audio Format:** 16kHz, 16-bit, Mono PCM WAV stream

## Hardware Wiring
The INMP441 is a mono microphone. By grounding the L/R word-select pin, the microphone is hardwired to output data exclusively on the Left Channel of the I2S stream.

| INMP441 Pin | ESP32-S3 Pin | Purpose |
| :--- | :--- | :--- |
| **VDD** | 3.3V | Power Supply |
| **GND** | GND | Common Ground |
| **SCK** | GPIO 4 | Serial Clock |
| **WS** | GPIO 5 | Word Select |
| **SD** | GPIO 6 | Serial Data |
| **L/R** | GND | Channel Selection (Left Channel) |

## Key Engineering Decisions
- **Native ESP-IDF Driver Adoption:** Migrated from the standard Arduino `I2S.h` library to the native ESP-IDF `driver/i2s.h`. This shift allows for direct, low-level manipulation of the I2S hardware controller, significantly reducing software overhead and eliminating kernel panics during continuous HTTP streaming.
- **Hardware-Level Channel Filtering:** Configured the I2S controller with `I2S_CHANNEL_FMT_ONLY_LEFT`. Instead of capturing stereo data and stripping the right channel in C++, this configuration drops the empty right channel at the hardware gate. This critical decision resolves severe browser timing issues, completely eliminating the 2x playback speed ("chipmunk" effect) and pitch distortion.
- **DMA Ring Buffer Management:** Implemented 8 Direct Memory Access (DMA) ring buffers (`dma_buf_count = 8`) to act as an asynchronous hardware shock absorber. When the processor is blocked by Wi-Fi transmission spikes, the DMA controller safely overwrites the oldest unread acoustic packets rather than queueing them infinitely. This prevents the compounding 25-second latency loops seen in standard queue structures, ensuring the stream remains truly "live."
- **Bit-Shifting & Payload Optimization:** The INMP441 captures audio in 32-bit frames. A bitwise right-shift operation (`>> 14`) is applied to downscale the raw telemetry into standard 16-bit WAV format. This operation simultaneously functions as a hardware-level gain control, truncating noise bits while optimizing the payload size for 8-bit TCP/IP transmission limits.

## Acceptance Criteria & Testing
The module is validated by connecting any standard Web Browser (Chrome, Safari, Edge) to the dynamically assigned local IP address of the ESP32-S3.

- **Playback Stability:** The browser correctly identifies the infinite HTTP WAV header and initiates a live media player. Audio plays at 1x speed with zero pitch distortion.
- **Latency Resilience:** Acoustic triggers (speech, clapping) are transmitted and heard with acceptable local-network latency. Simulated Wi-Fi stress tests confirm that the system successfully drops stale packets rather than buffering them, recovering instantly to a live state without restarting the stream.
- **Connection Handling:** Unwanted background browser requests (e.g., `favicon.ico`) are actively filtered and rejected with a `404 Not Found` header, preventing broken pipes and stream crashes.

## Author
**Tolga Han Kaya**  
[LinkedIn](https://www.linkedin.com/in/tolga-han-kaya-ccba%C2%AE-439a74136/) | [GitHub](https://github.com/TolgaHanKaya)

## Acknowledgments
- **AI Assistance:** Code scaffolding, syntax formatting, and documentation structuring were accelerated using AI tools, while architectural decisions, hardware integration, and QA testing were manually executed.