# ESL Project - PWM LED Blinking with GPIOTE Button Control

A Nordic nRF52840 firmware project featuring PWM-controlled LED blinking sequences with GPIOTE-based button handling and double-click detection.

## Features

- **PWM-based LED Control**: Smooth brightness ramping (0-100%-0%) at 1 kHz frequency
- **LED Sequence**: Cycles through LEDs 1, 2, 3, 4 (device ID: ABCD)
- **GPIOTE Button Handler**: Interrupt-driven button input on GPIO 11 with pullup
- **Double-Click Detection**: Toggle blinking on/off with double-clicks; single-click records timing
- **App Timer Integration**: Non-blocking PWM duty cycle updates every 10ms
- **Low-Frequency Clock**: Enabled for app_timer operation
- **MBR Support**: Compatible with nRF52840 MBR bootloader

## Hardware

- **Board**: PCA10059 (nRF52840 Dongle)
- **LEDs**: GPIO 13, 14, 15, 16
- **Button**: GPIO 11 (active low, pullup)

## Build Requirements

- ARM GCC toolchain (`arm-none-eabi-gcc`)
- Nordic nRF SDK (referenced as `../esl-nsdk`)
- GNU Make

## Building

```bash
cd /home/user/projects/ESL-project
make nrf52840_xxaa
```

Output: `_build/nrf52840_xxaa.out` (ELF)
Hex: `_build/nrf52840_xxaa.hex`
DFU Package: `_build/nrf52840_xxaa.dfu`

## Project Structure

```
.
├── main.c                  # Application firmware
├── Makefile               # Build configuration
├── config/
│   ├── sdk_config.h      # SDK feature configuration (from dongle_logs sample)
│   └── app_config.h      # App-specific configuration
├── armgcc/
│   └── blinky_gcc_nrf52.ld  # Linker script (MBR-aware)
└── README.md              # This file
```

## Architecture

### Clock & Timer
- **LFCLK**: Enabled via `nrf_drv_clock_lfclk_request()`
- **App Timer**: RTC-based periodic timer for PWM ramping (10ms period)

### PWM Module
- **Driver**: `nrfx_pwm` (PWM0 instance)
- **Frequency**: 1 kHz (16 MHz / 16000 counts)
- **Mode**: Individual channel control, looping
- **Update Rate**: 10ms per duty cycle step (smooth ramping)

### GPIOTE Button Handler
- **Driver**: `nrfx_gpiote` (GPIO Task & Event)
- **Polarity**: Low-to-High transition detection (button press)
- **Debounce**: Implicit (300ms double-click window)

### Blinking State Machine
```
[Stopped] --double-click--> [Blinking]
            (LED maintains current brightness)
            
[Blinking] --double-click--> [Paused]
            (LED holds brightness from before pause)
            
[Paused] --double-click--> [Blinking]
```

## Code Flow

1. **Initialization**:
   - Clock init → LFCLK request
   - PWM init → PWM0 configured, looping sequence enabled
   - App timer init → 10ms timer created
   - GPIOTE init → Button interrupt enabled

2. **Button Press** (GPIOTE interrupt):
   - Record timestamp
   - Check if within 300ms of last press
   - If double-click: toggle blinking
   - If single-click: store time for next press

3. **PWM Update** (App timer callback):
   - If blinking enabled:
     - Increment/decrement duty cycle by 50 steps
     - When max reached, switch to decrement phase
     - When 0 reached, advance to next LED in sequence
   - Update active LED channel with current duty cycle

## Configuration Notes

- **SDK Config Source**: `dongle_logs` sample provides logging backend config
- **USE_APP_CONFIG Flag**: Enabled in Makefile to use local app_config.h
- **Compiler Flags**: `-DUSE_APP_CONFIG -DBOARD_PCA10059 -DMBR_PRESENT`

## Current Build Status

⚠️ **Linker Issue**: NRFX driver symbols (`nrfx_pwm_init`, `nrfx_gpiote_init`) not resolving.
- All source files compile successfully
- Object files contain only debug metadata, no code symbols
- Likely cause: NRFX conditional compilation macros not evaluating correctly
- Status: Under investigation

## Future Improvements

- [ ] Resolve nrfx driver linking issue
- [ ] Add Bluetooth connectivity
- [ ] Support USB CDC logging
- [ ] Configurable PWM frequency and ramping speed
- [ ] Persistent settings (flash storage)

## License

Nordic Semiconductor SDK components are used under their respective licenses.

## References

- [nRF52840 Product Specification](https://infocenter.nordicsemi.com/)
- [nRF5 SDK Documentation](https://developer.nordicsemi.com/)
- [NRFX Drivers Documentation](https://infocenter.nordicsemi.com/index.jsp)
