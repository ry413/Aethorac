file(GLOB_RECURSE SOURCES "./*.c" "./*.cpp" "../../libs/xzbgs3_gui_task.c" "../../libs/st7701s.c")
file(GLOB_RECURSE UI_SOURCE "../ui/*.c" "../ui/*.cpp" "../../libs")

idf_component_register(SRCS
                        ${SOURCES}
                        ${UI_SOURCE}
                       INCLUDE_DIRS
                        "."
                        "../ui"
                        "../ui/components"
                        "../ui/fonts"
                        "../ui/screens"
                       )
spiffs_create_partition_image(spiffs spiffs_image FLASH_IN_PROJECT)