# Hobgoblin 

Small and simple game engine, intended to serve as a basis for small and simple games.

It is written in C++17 and uses [SFML](https://www.sfml-dev.org/) as the core for many of its features.

### Modules:
Hobgoblin provides its features across several mutually independent (with two exceptions) components:

 - **ChipmunkPhysics:** [A C-based physics engine](https://chipmunk-physics.net/) not written by me but only embedded
 within Hobgoblin, with some type-safety extensions included.
 - **ColDetect:** Facilities for efficient broad-phase and narrow-phase 2D collision detection. (NOTE: Narrow-phase not
 yet implemented)
 - **Graphics:** Various utilities for rendering stuff on-screen, loading sprites, packing them onto textures etc.
 - **Math:** Math stuff - geometry, trigonometry etc.
 - **QAO:** Event-driven framework for managing [active game objects](https://en.wikipedia.org/wiki/Active_object). 
 However, the traditional threaded approach is greatly simplified  in that it's designed to be single-threaded - 
 objects can implement certain "event" methods and the single-threaded QAO runtime calls them periodically in a well 
 defined order. It's left up to the implementer to ensure that these methods are mutually cooperative (that they always
 perform a small enough chunk of their jobs so that they return quickly and leave enough time for others).
 - **Preprocessor:** A bit of macro magic! (Rarely needed in modern C++, but occasionally useful.) This module is the
 first exception to the above rule, meaning that some other components import it and use it.
 - **RigelNet:** A networking library based on the 
 [remote procedure calling](https://en.wikipedia.org/wiki/Remote_procedure_call) paradigm made specifically for 
 peer-to-peer connections - where both the clients and the server will be executing code compiled from the same source.
 It uses either TCP (not yet implemented) or a custom-built reliable UDP protocol.
 - **Utility:** Miscellaneous generic utilities, such as custom containers, commonly used functions, multithreading
 utilities and other things. This package is the second exception mentioned in the above rule, in that other components 
 may (and do) depend on this one.
 
Each module has its own Readme file along with its Header files (which means in `include/Hobgoblin/<module name>` 
directory), describing it in more detail.
 
### Supported platforms:
Hobgoblin does not use any platform-specific extensions and uses CMake as its build system so, in theory at least, it
could support any platform supported by SFML, which means Windows, Linux, MacOS, iOS and Android. Currently it can be
compiled on Windows and MacOS, though Linux would probably also work.