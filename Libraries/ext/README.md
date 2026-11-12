Not used for now.
This is an alternative to builtin constants which should be loaded when extension loads
i.e. on `require 'ext/arduino.gpio'` we load the extension and then fallthrough to the file loading.
Extension loading is done in src/<arch>/builtins.c
