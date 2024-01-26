| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.

# esp-idf-atms

# Setup terminal:

- *For ESP32* In terminal start with: get_esp32 to set the right env variables -> It launches the esp-idf/export.sh behind.
- *For ESP8266* get_esp8266 instead


## List of useful commands

- Compile: idf.py build
- Flash on ESP: idf.py flash
- Monitor via Serial Monitor: idf.py monitor

# Relevant documentation
MFRC522 documentation: https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
PICC MIFARE Classic 1K - Mainstream contactless smart card IC documentation: https://www.mouser.com/datasheet/2/302/MF1S503x-89574.pdf
MIFARE Type Identification: https://www.nxp.com/docs/en/application-note/AN10833.pdf
MIFARE ISO/IEC 14443 PICC selection documentation: https://www.nxp.com/docs/en/application-note/AN10834.pdf
