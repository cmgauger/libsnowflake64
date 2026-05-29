# Snowflake64

An ANSI C, minimal dependency (only the BSD `tree.h` header!), configurationless
[Snowflake ID](https://en.wikipedia.org/wiki/Snowflake_ID) generator library.

## Building

First, initialise submodules:

```sh
git submodule update --init

make test
```

### MacOS notes

Install dependencies:

```sh
brew install cunit
```
