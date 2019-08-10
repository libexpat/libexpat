# Fuzzing

To enable the build of the fuzz targets, you need to run the configuration script with the following option.

```console
./configure --enable-fuzztargets
```

# Fuzzit integration

Travis will launch fuzzit regression jobs as part of travis continuous integration.
Script qa.sh is launched by Travis with different sanitizers.
For each of them, and for each target, a new regression job is created.
