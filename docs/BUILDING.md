# Building
1. Install the OpenOrbis toolchain release and set `OO_PS4_TOOLCHAIN`.
2. `make setup` (once) to copy the icon and runtime PRXs from a toolchain sample.
3. `make test` for the host tests, `make` for the package.
Title ID `BREW00099` is used for development; change `TITLE_ID`/`CONTENT_ID` in the Makefile.
