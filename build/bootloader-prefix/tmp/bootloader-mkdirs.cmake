# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/VSCodeHardWare/Espressif/frameworks/esp-idf-v5.2.3/components/bootloader/subproject"
  "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader"
  "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix"
  "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix/tmp"
  "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix/src"
  "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/DELL/Desktop/ESP32_TRY/FUKY_HardWare/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
