# Fuzzing

To enable the build of the fuzz targets, you need to run the configuration script with the following option.

```console
./configure --enable-fuzztargets
```

# Fuzzit integration

Travis will launch sanity fuzzit jobs as part of travis continuous integration.
Script qa.sh is launched by Travis wih different sanitizers.
For each of them, and for each target, a new sanity job is created.
The fuzzit target ids are stored in a configuration file fuzzitid.txt and used by qa.sh
