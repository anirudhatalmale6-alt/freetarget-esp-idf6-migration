# ---------------------------------------------------------------------------
# freeETarget (Development) - main component
#
# Updated for ESP-IDF 6.0.x
#
# What changed and why:
#
#  * The four absolute Windows include paths pointing into the IDF install
#    have been REMOVED. They are what broke hardest on 6.0 - see below.
#
#  * REQUIRES added. There was no REQUIRES list at all; the absolute paths
#    above were doing that job by hand. Naming a component in REQUIRES is how
#    you get its include directory - you never add -I paths into IDF yourself,
#    because the layout on disk is not part of the public interface and 6.0
#    moved most of it.
#
#  * The source glob was ${CMAKE_SOURCE_DIR}/main/*.* which swept every file
#    with a dot in its name into SRCS - .h, .html, .png, .json, .pem and the
#    stray main/build/CMakeLists.txt included. Narrowed to *.c.
#
#  * INCLUDE_DIRS listed "." twice, and "./tcpip" / "./html" in lower case.
#    The directories on disk are TCPIP and HTML. Windows does not care,
#    anything case-sensitive does - this was the first hard error.
#
#  * EMBED_FILES paths were likewise lower case, and "FreeEtarget" where the
#    files are "FreeETarget". The asm() symbol names in HTML/html.h expect
#    _binary_FreeETarget_html_start, i.e. the real spelling, so these are now
#    written the way they are on disk.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.c)

# TODO(IDF6): main/drivers/sockets.c is a verbatim copy of lwIP's own
# sockets.c (4613 lines, "This file is part of the lwIP TCP/IP stack").
# It #includes api_lib.c / api_msg.c / netbuf.c, which are lwIP-internal
# sources not on any include path, and every symbol in it collides with the
# real lwIP already linked in via the lwip component. Excluded from the build.
# It looks like a reference copy rather than something meant to compile.
list(FILTER app_sources EXCLUDE REGEX "/sockets\\.c$")

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS
                            "."
                            "./TCPIP"
                            "./drivers"
                            "./HTML"
                       REQUIRES
                            # core
                            log
                            esp_timer
                            esp_system
                            esp_hw_support
                            freertos
                            nvs_flash
                            # networking
                            lwip
                            esp_wifi
                            esp_netif
                            esp_event
                            esp_http_server
                            esp_http_client
                            esp-tls
                            mbedtls            # esp_crt_bundle.h, used by TCPIP/http_client.c
                            # OTA / partitions
                            app_update         # esp_ota_ops.h, esp_app_format.h
                            esp_partition
                            spi_flash          # esp_flash_partitions.h
                            # peripherals - 6.0 split the old monolithic
                            # "driver" and "hal" components per peripheral
                            driver             # driver/i2c.h (legacy i2c still lives here)
                            esp_driver_gpio
                            esp_driver_ledc
                            esp_driver_rmt
                            esp_driver_pcnt
                            esp_driver_uart
                            esp_driver_gptimer # replaces the removed legacy timer group API
                            esp_adc
                            esp_hal_gpio       # hal/gpio_types.h
                            esp_hal_ana_conv   # hal/adc_types.h
                            # managed components
                            espressif__led_strip
                            espressif__mdns
                       EMBED_FILES
                            "HTML/FreeETarget.html"
                            "HTML/FreeETargetIcon.png"
                            "HTML/help.html"
                            "HTML/menu.html"
                            "TCPIP/ca_cert.pem"
                       )
