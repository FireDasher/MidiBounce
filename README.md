# MidiBounce
[Midi Playground](https://github.com/quasar098/midi-playground), but in C++ and better

It was [Rewritten in Rust](https://github.com/FireDasher/midi-bounce) the Rust version is better

## Building
1. Download SFML from [here](https://www.sfml-dev.org/download/sfml/3.0.2/), put it somewhere cool like in your user directory, add the bin directory to Path environment variable, include to CPath, and lib to Library_Path. For other OS's and compilers it's probably different but you get to figure that out. You can also just use some command line arguments like `-L` for lib and `-I` for include instead of environment variables if you want.
2. `g++ main.cpp world.cpp MidiParser.cpp -std=c++20 -lsfml-system -lsfml-window -lsfml-graphics -lsfml-audio -o midibounce.exe` Replace g++ with your compiler if not using gcc

## Running
There should be a windows build in releases but on other OS's you need to build it yourself.  
Run midibounce without any paramters to see how to use it.  
Also sometimes it will freeze and become unresponsive while generating, if so then press CTRL+C to cancel generation then run the command again or if using a specific seed then try a different seed.
