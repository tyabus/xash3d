# Contributing to the project

Third-party patches for Xash3D SDL are great idea and highly welcomed. 
But if you want your patch to be accepted, we can give you few guidelines.

## If you are reporting bugs

1. Check you are using latest version. You can build latest Xash3D for yourself, look to README.md.
2. Check open issues is your bug is already reported and closed issues if it reported and fixed. Don't send bug if it's already reported.
3. Now you can write the bugreport.
4. Create issue per bug, mixing them is a really bad idea.
5. Write down steps to reproduce the bug.
6. Write down OS and architecture you are using.
7. Re-run engine with `-dev 5 -log` arguments, reproduce bug and post engine.log which can be found in your working directory.
8. Use debugger to provide a backtrace if the engine has crashed (please compile the engine with debug symbols).
9. Attach screenshot(s) if it will help clarify the sitation.
10. Wait for an answer.

## If you are contributing code

### Which branch?

* We recommend using the `master` branch. But you still can use any of our branches, if they are up-to-date.

### Third-party libraries

* Philosophy of Xash Project by Uncle Mike: don't be bloated. We follow it too.
* We only allow libraries only which are used in Linux GoldSrc version or if there is a REAL reason to use library.

### Portability level

* Xash3D have it's own crt library. It's recommended to use it. It most cases it's just a wrappers around standard C library.
* If your feature needs a platform-specific code, try to implement to every supported OS and every supported compiler.
* You must put it under appopriate macro. It's a rule: Xash3D SDL must compile on every platform we support.

| OS | Macro |
| -- | ----- |
| Linux | `defined(__linux__)` |
| FreeBSD | `defined(__FreeBSD__)` |
| NetBSD | `defined(__NetBSD__)` |
| OpenBSD | `defined(__OpenBSD__)` |
| OS X/iOS | `defined(__APPLE__)` and TargetConditionals macros |
| Windows | `defined(_WIN32)` |
| Android | `defined(__ANDROID__)` |
| Haiku | `defined(__HAIKU__)` |
| Sailfish | `defined(__SAILFISH__)` |

### Licensing

* Look COPYING file in repository, if you are unsure.

### Code style

* This project uses Quake's C code style convention. 
* In short:
  * Use spaces in parenthesis. Even in empty.
  * Use tabs, not spaces, for indentation.
  * Any brace must have it's own line.
  * Short blocks, if statements and loops on single line is allowed.
* You can use clang-format here. Config is located in `.clang-format` file in repository's root.
