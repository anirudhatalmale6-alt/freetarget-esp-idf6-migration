# freeETarget (Development) - ESP-IDF 5.x to 6.0 migration notes

Target: **esp32s3**. Verified against **ESP-IDF v6.0.2**.

Result: `idf.py build` completes from a clean tree, compiles and links.

```
freeETarget.bin binary size 0x145340 bytes. Smallest app partition is 0x200000 bytes.
0xbacc0 bytes (36%) free.
Project build complete.
```

Every change is tagged in the source with `TODO(IDF6):`. To find them all:

```
grep -rn "TODO(IDF6)" main CMakeLists.txt
```

There are 61 markers across 31 files. Most are explanatory - they say what
moved and why. **Five need a decision from you**; they are marked
`*** PLEASE LOOK AT THIS ONE ***` in the source and listed first below.

This package got the same treatment as `master`, so the four items there
apply here too and are not repeated in detail. There is **one extra item that
is specific to this branch** and it is the most important thing in this
document - it is item 5 below.

---

## Needs your review

### 1. `main/timer.c` - the 1 ms tick has been ported to a new driver

Your timer used the **legacy timer group driver** - `timer_init()`,
`timer_set_alarm_value()`, `timer_isr_callback_add()`, `TIMER_GROUP_0`,
`TIMER_1`. That API was deprecated years ago, survived in 5.x under
`components/driver/deprecated/`, and is **deleted outright in 6.0**. There is
no header to point at and no compatibility shim.

It is ported to `gptimer`. The timing is reproduced exactly rather than tidied
up:

| old | new |
|---|---|
| APB 80 MHz, `.divider = 16` | `resolution_hz = 5000000` |
| `ONE_MS` = 4960 counts | `alarm_count = 4960` |
| `.auto_reload = 1` | `flags.auto_reload_on_alarm = true` |
| `timer_set_counter_value(..., 0)` | `reload_count = 0` |

One thing worth knowing: `TIMER_SCALE` is `(1000 / TIMER_DIVIDER)` in integer
arithmetic, which truncates 62.5 to 62, so `ONE_MS` is 4960 counts = **992 us,
not 1000**. Your "1 ms" tick has always run about 0.8% fast. I kept 4960
because everything downstream is calibrated against the tick you actually
have. Setting the alarm to 5000 gives a true millisecond but shifts every
timer in the system by 0.8% - a deliberate change, not a bug fix.

`drivers/pcnt.c` and `NTP.c` also included `driver/timer.h` but never called
it. Removed from both.

### 2. `main/freETarget.c` - `find_sensor()` returns a string when it fails

```c
/*
 * Not found, return null
 */
return LED_READY;          /* "g----" from diag_tools.h */
```

`find_sensor()` returns `sensor_ID_t *`. `LED_READY` is a 6 byte string
constant. The comment says null; the code returns a string.

I did **not** change it to `NULL`. All 17 call sites dereference the result
immediately - `find_sensor(1 << i)->long_name` and similar - with no null
check, so returning `NULL` would turn a silent wrong answer into a crash. I
cast it instead, which keeps today's behaviour exactly.

The real fix is either to return `NULL` and add checks at the call sites, or
to return a static "unknown sensor" record. Your call.

### 3. `main/nonvol.c` - `nonvol_write_i32()` stored the pointer, not the value

```c
/* was */ nvs_set_i32(my_handle, name, value);    /* value is an int *  */
/* now */ nvs_set_i32(my_handle, name, *value);
```

`nvs_set_i32()` takes an `int32_t` **value**. As written, what went into NVS
was the *address* of the variable. GCC 15 makes this a hard error where it
used to be a warning.

**This is the identical bug that was in the Trace package.** Anything
previously written through this path is meaningless, so a factory reset may be
in order once you are running again.

### 4. `main/drivers/gpio.c` - a check in `status_LED_test()` is commented out

```c
if ( ((IS_HOLD_C(rapid_C_LED)) && (IS_HOLD_C(rapid_D_LED))) || ... )
```

`IS_HOLD_C(x)` expands to `(json_mfs_hold_c == (x))`. `json_mfs_hold_c` is an
int holding an MFS action code; `rapid_C_LED` and `rapid_D_LED` are
**functions**. So this compares an int against a function address and can
never be true - the warning it guards has never printed. Commenting it out
changes nothing that happens today.

It looks like it wants the action codes from `mfs.h`
(`IS_HOLD_C(MFS_C_LED)`), but the exact combination you intended is a guess,
so I left it for you. Diagnostic output only.

### 5. `main/json.h` - `json_rapid_time` and `json_rapid_wait` were `int`

**This one is a live bug, not just a compile error, and it is specific to this
branch.** Both were declared `EXTERN int`. Everything else in the package
treats them as `real_t` (double):

- `json.c` marks both rows `IS_FLOAT`, so the parser writes a **double**
  through them - 8 bytes into a 4 byte int, which corrupts whatever sits next
  to them in memory;
- `timed_fire.c` stores their addresses in `rapid_state_t.timer`, declared
  `real_t *`, and dereferences them as double;
- your `master` package declares both `real_t`, and prints them with `%4.2f`
  where this branch prints `%d`.

So `int` is the odd one out in four places. I changed them to `real_t` to
match, and changed the one debug `sprintf` in `timed_fire.c` from `%d` to
`%4.2f` accordingly (printing a double through `%d` is undefined behaviour,
not merely a wrong-looking number).

If you are deliberately converting these to integers in this branch, then the
change needs to go the other way: `json.c` wants `IS_INT32` on those two rows
and `rapid_state_t.timer` wants to be an `int *`. Tell me which way you want it
and I will do the rest.

---

## What actually broke, and why

### The absolute Windows include paths

`main/CMakeLists.txt` had four of these:

```cmake
"C:/esp/v6.0.2/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos"
"C:/esp/v6.0.2/esp-idf/esp-idf/components/hal/include/hal"      <- doubled "esp-idf"
"C:/esp/v6.0.2/esp=idf/components/esp_adc/include/esp_adc"      <- "esp=idf", equals sign
"C:/esp/v6.0.2/esp-idf/components/esp_http_server/include"
```

Two of them have typos, which I assume are from last week's debugging.

These point into IDF's internals, which are not a public interface, and 6.0
moved most of them - `components/hal` is now split into `esp_hal_gpio`,
`esp_hal_ana_conv` and friends. All four are removed.

There was also **no `REQUIRES` list at all**, because those paths were doing
that job by hand. Naming a component in `REQUIRES` is how you get its include
directory; it survives version changes, hardcoded paths do not. The new list is
in `main/CMakeLists.txt` with a comment explaining each addition.

### Component reorganisation

6.0 split the monolithic `driver` and `hal` components per peripheral.
`SOC_PCNT_UNITS_PER_GROUP` also disappeared from the public `soc_caps.h` with
no renamed equivalent anywhere in the 6.0 tree - it is defined locally in
`drivers/pcnt.c` as 4, which is what the ESP32-S3 has.

| header | 5.5 | 6.0 |
|---|---|---|
| `hal/gpio_types.h` | `components/hal/include/hal/` | `components/esp_hal_gpio/include/hal/` |
| `hal/adc_types.h` | `components/hal/include/hal/` | `components/esp_hal_ana_conv/include/hal/` |
| `driver/timer.h` | `components/driver/deprecated/driver/` | **gone** |
| `driver/adc.h` | `components/driver/deprecated/driver/` | **gone** |

### The managed components

`components/espressif__led_strip` was a vendored copy of v2.4.1, byte for byte
identical to the one already in `managed_components/`. It declares
`PRIV_REQUIRES "driver"`, which no longer supplies RMT in 6.0, so it failed on
`driver/rmt_tx.h`.

I removed the vendored copy and relaxed `main/idf_component.yml` from
`^2.4.1` to `>=2.5.5` so the component manager fetches a version that builds
against 6.0. `dependencies.lock` and `managed_components/` were deleted so they
re-resolve. `espressif/mdns` resolved cleanly with no change.

### Include paths

Backslash separators (`#include "driver\gpio.h"`), bare header names missing
their component sub-directory, and case-only mismatches that work on Windows
but not on a case-sensitive filesystem:

| was | now |
|---|---|
| `gpio_types.h` | `hal/gpio_types.h` |
| `adc_types.h` | `hal/adc_types.h` |
| `mpu_wrappers.h` | `freertos/mpu_wrappers.h` |
| `wifi.h` | `WiFi.h` |
| `ota.h` | `OTA.h` |

`INCLUDE_DIRS` also had `"./tcpip"` and `"./html"` where the directories are
`TCPIP` and `HTML`. That was the very first hard error - "not a directory".

### The compiler is now GCC 15, building as C23

This generated most of the error messages. Long-standing warnings are now
errors:

- **Empty parameter lists mean `(void)` in C23.** `http_send_string_end()` and
  `ft_timer_new()`'s callback argument both had prototypes that no longer
  match their definitions.
- **`-Werror=ignored-qualifiers`.** `time_count_t` is `typedef volatile
  int32_t`; a `volatile` return type is meaningless so it is discarded and now
  complained about. `run_time_seconds()` and `run_time_ms()` return `int32_t`.
  The typedef is untouched.
- **`-Werror=cast-function-type`.** Direct function-pointer casts are rejected;
  routed via `(void *)`, which is the usual way to keep them. No generated code
  changes. Affects the GPIO ISR table, the self-test table and `check_12V`.
- **`-Werror=incompatible-pointer-types`.** The `JSON[]` table stores several
  types in two generic columns - some rows were already cast, most were not.
  The rest are now cast the same way.
- **`-Werror=discarded-qualifiers`.** `mfs_find()`, `contains()`, `squish()`
  and the `rapid_state` pointer now say `const` where they read const data.
- **`-Werror=implicit-fallthrough`.** Deliberate fall-throughs in `gpio.c` and
  `json.c` marked with `__attribute__((fallthrough))`, which emits no code.
- **`-Werror=missing-field-initializers`.** The `sensor_t s[4]` table uses
  designated initializers now. The omitted fields are still zero-initialised
  exactly as before.
- **`-Werror=switch`.** 6.0 added `HTTP_EVENT_ON_STATUS_CODE` and
  `HTTP_EVENT_ON_HEADERS_COMPLETE`; the handler has a `default:` that ignores
  them, which is what happened before they existed.
- **`-Werror=attributes`.** `IRAM_ATTR` on both the declaration and the
  definition of the timer ISR asks for two different `.iram1.N` sections. It is
  on the definition only now, so the ISR still lives in IRAM.
- **`&array` where `array` was wanted.** `target_name(&str_c)`,
  `WiFi_remote_IP_address(&str_c)`, `get_OTA_serial(..., &ota_write_data)` and
  the three `&rapid_state_*` entries all passed a pointer-to-array where a
  pointer-to-element was expected. Same address, so nothing changes at runtime.
- **`int32_t` is `long int` on xtensa.** `nvs_get_i32()` would not take an
  `int *`. Same width, different type. This is why the S3 trips where the C3
  in your Trace package did not.

### Build system

`main/CMakeLists.txt` globbed `main/*.*`, sweeping `.h`, `.html`, `.png`,
`.json` and `.pem` into `SRCS`. Narrowed to `*.c`.

`main/drivers/sockets.c` is **excluded from the build**. It is a verbatim copy
of lwIP's own `sockets.c` - 4613 lines - which `#include`s `api_lib.c`,
`api_msg.c` and `netbuf.c`, lwIP-internal sources not on any include path, and
every symbol in it collides with the real lwIP linked in via the `lwip`
component. Same situation as in Trace.

---

## Things I did not change

`sdkconfig` is rewritten by the build itself when 6.0 opens a 5.3.1 config -
renamed and removed Kconfig symbols, plus auto-derived `CONFIG_SOC_*` values.
I checked your deliberate settings individually and they all survive:
custom partition table, `partitions.csv`, 8 MB flash, `FREERTOS_HZ=100`,
log level, target.

Worth knowing: **do not run `idf.py set-target` on this project.** It
regenerates `sdkconfig` from defaults and silently loses the custom partition
table and the 8 MB flash size, which then shows up as "app partition is too
small" - a confusing way to find out. The target is already in `sdkconfig`;
just run `idf.py build`.

---

## Rebuilding on your machine

```
idf.py fullclean
idf.py build
```

`fullclean` matters - `build/` carries cached CMake state from the 5.x
toolchain, and stale cache produces errors that look like source problems.
