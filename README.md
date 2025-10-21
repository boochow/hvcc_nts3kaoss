# hvcc External Generator for NTS-3 kaoss pad

This project is an external generator for [hvcc](https://github.com/Wasted-Audio/hvcc). It generates code and other necessary files for the KORG [logue SDK for NTS-3 kaoss pad](https://github.com/korginc/logue-sdk/tree/main/platform/nts-3_kaoss) from a Pure Data patch.

## Installation

Clone this repository, and ensure that both hvcc and the logue SDK are installed. You also need GCC/G++ in your development environment to estimate the required heap memory size (see the Appendix for details).

## Usage

1. Add the `hvcc_nts3kaoss` directory to your `PYTHONPATH` by running:

   ```bash
   export PYTHONPATH=$PYTHONPATH:path-to-hvcc_nts3kaoss
   ```

1. To convert your patch, run:

   ```bash
   hvcc YOUR_PUREDATA_PATCH.pd -G nts3kaoss_genfx -n PATCH_NAME -o DESTINATION_DIR
   ```

   Optionally, you can use another external generator, `nts3kaoss_oscfx`, to convert oscillator patches that use built-in parameters `pitch`, `pitch_note`, `slfo`, `noteon_trig`, and `noteoff_trig`.

   If your patch needs to keep accessing the input sound even when the touch pad isn't being touched, use `nts3kaoss_bgfx`. This allows the effect units to use `get_raw_input()` API to obtain a reference to the raw audio input buffer.

1. Check `DESTINATION_DIR`; it should contain four directories named `c`, `hv`, `ir`, and `logue_unit`.
   Move the directory named `logue_unit` under the logue SDK platform directory `logue-sdk/platform/nts-3_kaoss`.

1. In the `logue_unit` directory, run:

   ```bash
   make install
   ```

   Alternatively, you can specify your platform directory path by using a compile-time option:

   ```bash
   make PLATFORMDIR="~/logue-sdk/platform/nts-3_kaoss" install
   ```

## Examples

A separate repository containing sample patches for this project is available at:

[https://github.com/boochow/nts3kaoss_hvcc_examples](https://github.com/boochow/nts3kaoss_hvcc_examples)

## Receiving Parameters in Your Pure Data Patch

#### Fixed Parameters and Built-in Parameters

A set of built-in parameters is provided to receive touch pad events. These events carry x-y coordinates indicating where the touch event occurred.

While the logue SDK for NTS-3 has no fixed parameters, the `nts3kaoss_oscfx` external generator introduces several built-in parameters to help in implementing oscillator-type FX units.

| name             | type       | format           | description                                 |
| ---------------- | ---------- | ---------------- | ------------------------------------------- |
| touch_began      | built-in   | `f f` for x, y * | sent when a new touch was detected.         |
| touch_moved      | built-in   | `f f` for x, y * | sent while touching on the X-Y pad.         |
| touch_ended      | built-in   | `f f` for x, y * | sent at the end of a touch.                 |
| touch_stationary | built-in   | `f f` for x, y * | used to force-refresh current coordinates.  |
| touch_cancelled  | built-in   | `f f` for x, y * | sent when a touch forcibly ended.           |
| sys_tempo        | built-in   | `f`              | sent when a tempo change occurs.            |
| metro_4ppqn      | built-in   | bang             | sent when a clock event (16th note) occurs. |
| pitch            | fixed**    | `f`              | a MIDI note frequency in Hz.                |
| slfo             | fixed**    | `f`              | a unipolar LFO.                             |
| pitch_note       | built-in** | `f`              | a MIDI note number (integer).               |
| noteon_trig      | built-in** | bang             | sent when a new touch was detected.         |
| noteoff_trig     | built-in** | bang             | sent at the end of a touch.                 |

*Both `x` and `y` are integers ranging from 0 to 1023.

**These parameters are available only for `-G nts3kaoss_oscfx`.

 Any `[r]` object that includes `@hv_param` but is not listed above is recognized as a parameter. For NTS-3, up to eight parameters can be used. The pitch, pitch_note, slfo, noteon_trig, and noteoff_trig are available only for `nts3kaoss_oscfx` generator.

#### Specifying Min, Max and Default Values

By default, all variables receive raw integer values from the logue SDK API. You can specify a minimum value, a maximum value, and a default value like this:
`[r varname @hv_param 1 5 3]`

When the minimum, maximum, and default values are omitted, they are assumed to be `[0 1 0]`. The default value must be specified when the minimum and the maximum values are specified. 

The range of integer parameter values is limited to the range between -32768 and 32767. When min and max values exceed these limits, they are clipped to between -32768 and 32767.

#### Receiving Floating-Point Values

A variable with the postfix `_f` receives floating-point values between 0.0 and 1.0. The values are mapped from integer values between 0 and 1023. You can optionally specify the minimum, maximum, and default values using the syntax:

```
[r varname @hv_param min max default]
```

When min and max values are specified, values are mapped from integer values between 0 and 1023.

#### Parameter Slot, Default Assignment, and Curve Type

Optionally, you can specify the parameter slot, assignment to an input device, and the curve type that maps input coordinates to parameter values by adding a prefix `_NDC_` to the variable name. The remainder of the name is used as the variable name on the display. 

The `N`, a number from `1` to `8`, specifies which parameter slot the parameter is assigned to. The `D`, one of `x`, `y`,`z` (case insensitive), means one of the PAD X-axis, PAD Y-axis, or FX DEPTH. The `C` value can be one of: `a`(EXP), `b`(LOG), `c`(LINEAR), `d`(TOGGLE), `r`(MINCLIP), or `l`(MAXCLIP). The `C` values in lower cases mean UNIPOLAR curves and upper cases mean BIPOLAR curves.

For example, the variable name `_3zb_ratio` assigns the parameter "ratio" to slot 3, and the values are mapped linearly from the FX DEPTH values.

Parameters without `N` are assigned to one of the remaining empty parameter slots. The default values of curve type and polarity are LINEAR UNIPOLAR.

### Making Oscillator-type Units

The additional external generator `nts3kaoss_oscfx.py` is also included in this repository to provide an easier way to implement oscillator-type units in Pure Data. The differences between `nts3kaoss_genfx` and `nts3kaoss_oscfx` are:

1. You can use additional fixed parameter `pitch`, `slfo`, built-in parameters `pitch_note`, `noteon_trig`, and `noteoff_trig`.
2. The first parameter slot is used for "Pitch" when you use `pitch` parameter in your patch. Another parameter slot is assigned to "LFO Rate" when the `slfo` parameter is used in your patch.
3. Both single and dual output channels are allowed. (`[dac~ 1]` or`[dac~]`).

The values of the `pitch` parameter are shown as note names on the display, and actual values are note numbers (7 bits) with 3 bits of fractional part. The `pitch` parameter receives floating-point values calculated from the note numbers (7 + 3 bits). The `pitch_note` parameter receives the integer part (7 bits) of the note numbers.

## Restrictions

### DAC and ADC

The logue SDK units operate only at a 48 kHz sampling rate. For `-G nts3kaoss_genfx`, the `[dac~]` and `[adc~]` objects must each have two channels. With `-G nts3kaoss_oscfx`, `[dac~]` can have a single channel, and its output data is duplicated to both output channels.

### Memory Footprint

A unit must fit within the [Max RAM load size](https://github.com/korginc/logue-sdk/tree/main/platform/nts-3_kaoss#supported-modules). Even if the binary exceeds this limit, no build error will occur, but a warning message below will appear.

```bash
 Memory footprint: 39796 bytes
 WARNING: Memory footprint exceeds 32768
```

### `msg_toString()` does not work

To reduce the memory footprint, `hv_snprintf()` is replaced by an empty function. The `msg_toString()` function of `hvcc` does not work because it requires `hv_snprintf()`.

## Appendix

### Size of the Heap Memory

To ensure that your unit will fit within the memory space limitation, you must specify the heap size of your unit before the build process. The heap size can be set as a compiler flag:

```makefile
-DUNIT_HEAP_SIZE=3072
```

The heap size estimation process is integrated in `config.mk`, or you can also specify it manually like this:

```bash
make HEAP_SIZE=4096
```

For automatic estimation, this external generator creates `testmem.c` and `Makefile.testmem`. When GCC and G++ are available, `testmem.c` is built and executed by `config.mk`, and the estimated heap size will be saved in `logue_heap_size.mk`.

You can check the `malloc()` calls and the total requested memory for generating the first 960,000 samples by running:

```bash
make -f Makefile.testmem
./testmem
```

If GCC and G++ are not available in your development environment, the default heap size (3072 bytes) is used. (Note: In the Docker image version of the logue SDK build environment, GCC/G++ are not provided, so the default heap size is always used.)

### Size of the SDRAM allocation

The source code generated by this external generator allocates memory blocks whose sizes are larger than 256 bytes in the SDRAM area. This feature is enabled for the modfx, delfx, and revfx units to store large data structures such as delay lines.

Since the logue SDK requires units to allocate SDRAM blocks in their initialization process, you must specify the total SDRAM size of your unit before the build process. The size for your unit is estimated within the same process as heap memory size estimation, so you usually don't need to estimate it manually.

### Math Functions Approximation

Some math functions have been replaced with logue SDK functions that provide approximate values. If you get inaccurate results, comment out the following line in `config.mk`:

```makefile
UDEFS += -DLOGUE_FAST_MATH
```

to disable the fast math approximation. Note that disabling this may result in a larger binary size.

### Internal Sampling Rate

Enabling the `-DRENDER_HALF` option lets the oscillator unit calculate only half of the requested sample frames and interpolate the rest. This results in the sound lacking harmonics above 12 kHz, and many artifacts may appear in higher notes if no band-limiting technique is used. You can edit the config.mk file and enable the commented line:


```makefile
UDEFS += -DRENDER_HALF
```

to turn on this feature.

### Filling a Table with White Noise

Any table whose name ends with `_r` and is exposed using the `@hv_table` notation is always filled with white noise using the logue SDK function `fx_white()`. This feature can be used to replace the `[noise~]` object with the much lighter `[tabread~]` object.

However, note that:

1. Filling the buffer with `fx_white()` requires significant processing time, so keep the table size as small as possible (typically 64 samples).
2. To generate white noise, you also need a `[phasor~ freq]`, `[*~ tablesize]`, and `[tabread~]` object. The value of `freq` should be `48000 / tablesize`.

## Credits

* [Heavy Compiler Collection (hvcc)](https://github.com/Wasted-Audio/hvcc) by Enzien Audio and maintained by Wasted Audio
* [logue SDK](https://github.com/korginc/logue-sdk) by KORG
* [Pure Data](https://puredata.info/) by Miller Puckette
