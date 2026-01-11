Primary repository on [GitHub](https://github.com/CyberBotX/DiceServ).

# DiceServ for Anope 2.0

This is DiceServ, a dice rolling service for version 2.0 of the [Anope IRC Services](http://www.anope.org/). It's primary purpose is for tabletop role-playing games being played over IRC. It consists of the following commands:

* ROLL (basic dice rolls)
* EXROLL (extended output on dice rolls)
* CALC (like ROLL but without rounding)
* EXCALC (like EXROLL but without rounding)
* DND3ECHAR (rolls stats for a Dungeons and Dragons 3rd Edition character)
* EARTHDAWN (rolls dice based off of Earthdawn's step table)
* SET IGNORE (allows DiceServ to ignore usage by users or in channels)
* STATUS (for Services Operators only, allows them to view the status of a channel or user)
* LIST (for Services Operators only, allows them to list the ignored/allowed status of channels or users)

These commands can be called on DiceServ directly or be called in a channel through BotServ fantasy commands, such as !roll for example.

The parser supports not only dice rolls, but also much more complex math. It has support for:

* The constants e and pi
* Many math functions (examples include abs, floor and sin)
* Addition, subtraction, multiplication, division, powers (using ^) and modulus
* Implicit multiplication
* Unary minus (negative numbers)
* Percentile dice
* Rolling multiple sets of the same dice

DiceServ was originally created for Epona 1.4.14 in 2004. Version 2 of DiceServ was created as a module for Anope 1.8/1.9 in 2011, with all functionality in a single file. Version 3 of DiceServ was created as a set of modules for Anope 2.0 in 2016, heavily modularizing the service into multiple modules.

DiceServ includes its own random number generator (RNG), which is a double-precision SIMD-oriented Fast Mersenne Twister RNG, with the code coming from [Mutsuo Saito and Makoto Matsumoto](http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/). It parses by taking the given infix-notation expression and converting it into postfix-notation using the [Shunting-yard algorithm](https://en.wikipedia.org/wiki/Shunting-yard_algorithm), with an improvement to handle arbitrary numbers of arguments on functions coming from [Robin Sheat's blog](https://blog.kallisti.net.nz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/).

## Compiling and Installing

To compile DiceServ for use with Anope, place all of DiceServ's files into their own directory in the modules/third directory. (NOTE: The files **MUST** be in their own directory for all the modules to get compiled correctly.) Once you have done this, when you re-configure Anope's build process, it will find DiceServ and set it to compile on the next `make` and it will install when `make install` is run.

## Configuration

By default, there will be a diceserv.example.conf file placed in Anope's conf directory. You can use this as-is to get a DiceServ client on the network and all the above commands will be available. If you wish to customize DiceServ, it is recommended that you copy this file to diceserv.conf and edit the file accordingly. Regardless of which way you go, you must make sure to include the configuration file from Anope's main configuration file, typically using an `include` block.

## Usage

Once DiceServ has been included in your configuration, it should be available for use the next time you start Anope (or reload its configuration if it was already running). Commands are sent to the DiceServ client defined in the configuration set above. It is recommended that you also configure your IRC daemon's aliases so that the DS and DICESERV commands will be routed to the DiceServ client. Examples of how to use the various DiceServ commands can be found by sending the text `HELP` to DiceServ.

## Contact

If you have any questions, comments or concerns about DiceServ:

* Contact me by email: cyberbotx@cyberbotx.com (please include DiceServ in the subject line)
* Contact me on IRC: join the channel #diceserv on the server jenna.cyberbotx.com
* Submit an issue via [GitHub's issue tracker](https://github.com/CyberBotX/DiceServ/issues)

## License

With the exception of the Anope-specific parts of DiceServ (which are under the GPLv1 license) and the RNG (which is under the BSD 3-clause license), the remaining parts of DiceServ are licensed as follows:

```
The MIT License (MIT)

Copyright (c) 2004-2017 Naram Qashat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
