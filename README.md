## MinHTTP

[![Linux](https://github.com/asimos-bot/minhttp/actions/workflows/test-linux.yml/badge.svg)](https://github.com/asimos-bot/minhttp/actions/workflows/test-linux.yml)
[![MacOS](https://github.com/asimos-bot/minhttp/actions/workflows/test-macos.yml/badge.svg)](https://github.com/asimos-bot/minhttp/actions/workflows/test-macos.yml)
[![Windows (MSVC)](https://github.com/asimos-bot/minhttp/actions/workflows/test-windows.yml/badge.svg)](https://github.com/asimos-bot/minhttp/actions/workflows/test-windows.yml)
[![No Dependencies](https://github.com/asimos-bot/minhttp/actions/workflows/no-includes.yml/badge.svg)](https://github.com/asimos-bot/minhttp/actions/workflows/no-includes.yml)

Minimal HTTP 1.0 and 1.1 parser and builder for requests and responses.

* no memory allocations
* no dependencies

### How to Use

There is two options:
* Build library and include it in your project
* Just copy `minhttp.c` and `minhttp.h` to the proper locations in your project.

### How to Build

1. `mkdir build/`
2. `cd build/`
3. `cmake .. && cmake --build .`

### How to Test

In `build/`: `./test` or `./test --quiet` (for no output on success).

The tests are based on the ones from [picohttpparser](https://github.com/h2o/picohttpparser/blob/master/test.c). Be aware that changes have been made due to difference between them:
* picohttpparser allows "keyless" values. This project doesn't 
    * compare their `"parse multiline"` test and this project's `"parse_headers_multiline_success_example_test"` and `"parse_headers_multiline_example_test"` for a better visualization.

### Roadmap

- [x] parsers
    - [x] pass all tests for request first line from picohttpparser
    - [x] pass all tests for header parser from picohttpparser
    - [x] pass all tests for response first line parser from picohttpparser
- [ ] builder
    - [ ] header builder
    - [ ] response first line builder
    - [ ] response builder
    - [ ] request first line builder
    - [ ] request builder
- [ ] benchmarking and optimizations
    - [x] benchmark against other http parsers
        - [x] picohttpparser
    - [ ] separate benchmark for header parsing and for first line parsing
    - [ ] straight-forward parsing with no state machine
        - [ ] get `key_begin` by being index after newline
        - [ ] get `key_len` by being everything before first colon
        - [ ] get `value_begin` by being first non whitespace after colon
        - [ ] get `value_len` by checking whitespaces before the next newline
    - [ ] enable NULL arguments for faster parsing
    - [ ] add likely and unlikely in all appropriate jumps
    - [ ] add code coverage percentage
    - [ ] parse only requested headers (`mh_parse_headers_set`)
        - [ ] get max key len automatically

### Header Parser State Machine

![Header Parser State Machine](./header-parser-state-machine.svg)

|        x           |       Line Start      |       First String        |     After Key     |       During Value       |         After Whitespace          |  DONE   |
|--------------------|-----------------------|---------------------------|-------------------|--------------------------|-----------------------------------|---------|
|Line Start          |                       |    other(start of key)    |                   |                          |                                   | newline |
|First String        |                       |           other           | colon(end of key) |                          |                                   |         |
|After Key           | newline(empty value)  |                           |    whitespace     |    other(start of value) |                                   |         |
|During Value        | newline(end of value) |                           |                   |        other/colon       | whitespace(possible end of value) |         |
|After Whitespace    | newline(end of value) |                           |                   |          other           |             whitespace            |         |


|        x           |  Newline   |      Whitespace      |    Colon     |       Other       |
|--------------------|------------|----------------------|--------------|-------------------|
|Line Start          |    DONE    |                      |              |   First String    |
|First String        |            |                      |  After Key   |   First String    |
|After Key           | Line Start |      After Key       |              |   During Value    |
|During Value        | Line Start |   After Whitespace   | During Value |   During Value    |
|After Whitespace    | Line Start |   After Whitespace   | During Value |   During Value    |
