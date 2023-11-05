# studyTimer
Software for a device that measures study time

## Clone

```
git clone --recursive https://github.com/wurly200a/studyTimer.git
```

or

```
git clone https://github.com/wurly200a/studyTimer.git
git submodule update --init --recursive
```

## Build

```
cd studyTimer
docker run --rm -it -v ${PWD}:/home/builder/studyTimer -w /home/builder/studyTimer wurly/builder_esp32_esp-idf-v4
```

```
get_idf
idf.py set-target esp32
kconfig-tweak --file sdkconfig --disable CONFIG_ESPTOOLPY_FLASHSIZE_2MB
kconfig-tweak --file sdkconfig --enable CONFIG_ESPTOOLPY_FLASHSIZE_4MB
kconfig-tweak --file sdkconfig --set-str CONFIG_ESPTOOLPY_FLASHSIZE "4MB"
kconfig-tweak --file sdkconfig --set-val CONFIG_FREERTOS_HZ 1000
kconfig-tweak --file sdkconfig --disable CONFIG_ESP32_RTC_CLOCK_SOURCE_INTERNAL_RC
kconfig-tweak --file sdkconfig --enable CONFIG_ESP32_RTC_CLOCK_SOURCE_INTERNAL_8MD256
idf.py build
```

## Write flash

```
~/.local/bin/esptool.py -p /dev/ttyUSB0 -b 460800 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/study_timer.bin
```
