file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)
idf_component_register(SRCS ${app_sources} 
                    INCLUDE_DIRS "."
                    "." 
                    "./tcpip" 
                    "./drivers"
                    "./html"
                    "C:/esp/v6.0.2/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos"
                    "C:/esp/v6.0.2/esp-idf/esp-idf/components/hal/include/hal"
                    "C:/esp/v6.0.2/esp=idf/components/esp_adc/include/esp_adc"
                    "C:/esp/v6.0.2/esp-idf/components/esp_http_server/include"
                    
                    EMBED_FILES 
                    "html/FreeEtarget.html"
                    "html/FreeEtargetIcon.png"
                    "html/help.html"
                    "html/menu.html"
                    "tcpip/ca_cert.pem"
                    )

