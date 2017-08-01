# A simple JSON parser and printer in C.

You may use this program in two ways:

*   Parse a C-string (char array with a NUL at the end) and use the
    structurized data in the program.
*   Store your own data in the structures and then print it out in
    JSON format.

## Notes

*   This program doesn't handle `undefined'.
*   Probably doesn't support multi-byte.
*   The `compound' struct isn't very memory-efficient in terms of bytes,
    but I find that you probably won't need a lot of compound for just
    processing normal data. (Or, you could just increase the size macros
    at the top.)
*   Memory allocation & error handling needs more love.
