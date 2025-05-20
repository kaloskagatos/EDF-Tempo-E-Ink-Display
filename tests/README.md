# Desktop Test Harness

This directory contains a small CMake project that allows running parts of the
application on a desktop computer. It builds the executable `TempoApp` which
mimics the Arduino build but relies on cURL for HTTP requests.

Running the executable without arguments reproduces the previous behaviour and
queries the real Tempo APIs:

```bash
./TempoApp
```

For unit tests that do not rely on network access, pass the `--unittest` flag:

```bash
./TempoApp --unittest
```

The unit tests use `StubTempoColorService` to provide fake data and verify the
logic of `TempoColorServiceManager` and other components.
