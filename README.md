# My Malloc

This is an experiment. I set out to learn more about how memory management is accomplished on Linux systems. Specifically, I wanted to do it in C. I aim to mimic the bare-bones functionality of GLIBC's malloc.

Again, this is a learning experience -- and as of now this code shouldn't really be used or relied on.

`valgrind` shows no errors at this point (with what's done in [main.c](https://github.com/c650/my-malloc/blob/master/src/main.c))

---
## Usage

Please refer to the information in [my-malloc.h](https://github.com/c650/my-malloc/blob/master/src/my-malloc.h).
The comments in that file are where a new user should look if he/she intends to use this code. ***Disclaimer:*** as I am not a "pro" and this code is not technically "production," it is not recommended that this code is used in place of GLibC's `malloc()`.

---
## Notice

Anyone is free to use this code, but I am not liable for any of its shortcomings. See the [LICENSE](https://github.com/c650/my-malloc/blob/master/LICENSE) for more information regarding usage and rights.
