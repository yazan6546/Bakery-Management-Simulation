cmake_minimum_required(VERSION 3.11.2)
project(my_raylib_game C)
set(CMAKE_C_STANDARD 99)

set(LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

# Adding our source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/sources/*.c") # Define PROJECT_SOURCES as a list of all source files


# Declaring our executable


add_executable(main src/main.c src/utils/config.c src/game.c src/inventory.c src/graphics/assets.c)
add_executable(graphics)
target_sources(graphics PRIVATE src/graphics/graphics.c)

add_executable(chefs src/chefs/chef.c)
add_executable(bakers src/bakers/baker.c)
add_executable(sellers src/sellers/seller.c)
add_executable(supply_chain src/supply_chains/supply_chain.c)
add_executable(customers src/customers/customer.c)


target_include_directories(main PRIVATE "include/lib/raylib") # Include the header files in the include directory for the main executable
include_directories(include)
target_include_directories(graphics PRIVATE include/lib/raylib)
target_link_libraries(main PRIVATE "${LIBRARY_DIR}/libraylib.a" m dl pthread) # Link the raylib static library and other required libraries
target_link_libraries(graphics PRIVATE "${LIBRARY_DIR}/libraylib.a" m dl pthread) # Link the raylib static library and other required libraries

set (need_queue main bakers customers graphics) # List of executables that need the queue library
foreach (need IN LISTS need_queue) # Loop through each executable that needs the queue library
    message("Adding ${need} to the main executable")
    target_include_directories(${need} PRIVATE include/lib/queue) # Add the source files for each executable to the main executable
    target_link_libraries(${need} PRIVATE ${LIBRARY_DIR}/libgenericQueue.a) # Add dependencies for the main executable
endforeach()
# Setting ASSETS_PATH

if (CMAKE_RELEASE_BUILD)
    message("Release build")
    target_compile_definitions(graphics PUBLIC ASSETS_PATH="./assets") # Set the asset path macro in release mode to a relative path that assumes the assets folder is in the same directory as the game executable
else()
    message("Debug build")
    target_compile_definitions(graphics PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/") # Set the asset path macro to the absolute path on the dev machine
    target_compile_definitions(main PUBLIC CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.txt") # Set the config path macro to the absolute path on the dev machine

endif()
