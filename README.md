### PH8 : A further development of PH7 PHP Engine. 

**Work in progress** [A roadmap](https://github.com/vvb333007/PH7-ESP32/blob/master/TODO.md)

**Comparision: PH8 vs ZendPHP**: [current status](https://github.com/vvb333007/PH8/blob/master/docs/Language.md)

**Сравнение PH8 и ZendPHP**: [состояние дел](https://github.com/vvb333007/PH8/blob/master/docs/Language_RU.md)


Compiles and runs on ESP32-family, Linux and Windows (Cygwin).
Memory footprint: requires about 100KiB of memory for the Engine plus ~180 KiB per virtual machine;


To play on Windows:

  * Make sure you have Cygwin installed
  * Clone the PH8 repository
  * Go to PH8/src/examples/linux_windows_example/
  * execute `make`
  * Execute `inter.exe` with a PHP file as an argument
  * File `PH8/Libraries/exc/sqlite` contains SQLite3 class implementation and must be `include`d by .php script




