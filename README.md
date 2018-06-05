# Blinky Tetris

Years ago my friend Benny made a 20 foot tall rectangle of 1,250 3-color LEDs
connected in serial. The LEDs were controlled by p9813 chips, which are very
easy to use because they don't rely on strict timing the way the ws2812 chips
do. Anyway, since he had a large rectangular multicolored screen, I decided it
would be fun to implement a Tetris game on that screen. I thought it would
also be fun to use large buttons for the controllers and put the controls far
apart from one another so people could play cooperatively.

It made its first appearance at the 2013 Maker Faire in San Mateo, and here is
some video (sorry for the noise):

[![Blinky Tetris at Maker Faire
2013](https://img.youtube.com/vi/DV-Rf6vygNQ/0.jpg)](https://www.youtube.com/watch?v=DV-Rf6vygNQ)

Anyway, I decided it would be fun to release the code I used if anyone wants
to do anything similar. This was designed to run on a Beaglebone, but a
raspberry pi also works. The only caveat is that you must enable to SPI bus.

Please look at the [elinux-tcl](https://github.com/CoolNeon/elinux-tcl)
library or the [arduino-tcl](https://github.com/CoolNeon/arduino-tcl)
libraries for more information about wiring up this board. I include the
elinux-tcl library within this distribution, though I believe it may be an
earlier version of the library.

If you do make something cool as a result of this code, please feel free to
open an issue and send me a link or a photo.
