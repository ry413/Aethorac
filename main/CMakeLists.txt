file(GLOB_RECURSE SOURCES ./*.cpp)

idf_component_register(SRCS
                        ${SOURCES}

                        "../components/wifi/wifi.cpp"
                        "../components/my_mqtt/my_mqtt.cpp"
                        INCLUDE_DIRS
                        "."
                        )