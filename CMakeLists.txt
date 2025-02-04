cmake_minimum_required(VERSION 3.31)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# prevent installing to system directories. 
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

# Declare the project
project(chip8)

# Set the name of the executable
set(EXECUTABLE_NAME ${PROJECT_NAME})

add_executable(${EXECUTABLE_NAME})

# Add your sources to the target
target_sources(${EXECUTABLE_NAME} 
PRIVATE 
    src/main.cpp
    src/chip8.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp 
    imgui/imgui_tables.cpp 
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl3.cpp 
    imgui/backends/imgui_impl_sdlrenderer3.cpp
)

# Set C++ version
target_compile_features(${EXECUTABLE_NAME} PUBLIC cxx_std_20)

target_include_directories(${EXECUTABLE_NAME} PUBLIC imgui)
target_include_directories(${EXECUTABLE_NAME} PUBLIC imgui/backends)

# Configure SDL by calling its CMake file.
# we use EXCLUDE_FROM_ALL so that its install targets and configs don't
# pollute upwards into our configuration.
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# Link SDL to our executable. This also makes its include directory available to us. 
target_link_libraries(${EXECUTABLE_NAME} PUBLIC 
    SDL3::SDL3              # If using satelite libraries, SDL must be the last item in the list. 
)
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)
