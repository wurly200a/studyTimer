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
esptool.py --chip esp32 --port <<port>> --baud 921600 write_flash 0x1000 bootloader.bin 0x8000 partition-table.bin 0x10000 study_timer.bin
```
