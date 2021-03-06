# Bazel Guide
## Install Bazel on Linux
1. Download Bazel 4.0.0
   ```sh
   wget -O bazel https://github.com/bazelbuild/bazel/releases/download/4.0.0/bazel-4.0.0-linux-x86_64
   ```
   > Note: If you are using proxy don't forget to set
   `http_proxy` and `https_proxy` environment variables

2. Put it somewhere on **local disk** (use of remote shared
   drives is not recommended) and set executable attributes.
   ```sh
   mv bazel <my-user-dir-utils>/bin
   chmod +x bazel
   ```

3. Create Bazel cache directory on **local disk** and
   configure path to the cache in `~/.bazelrc`.
   ```sh
   mkdir -p <my-user-dir-on-local-disk>/.bazel-cache
   echo "startup --output_user_root=<my-user-dir-on-local-disk>/.bazel-cache" > ~/.bazelrc
   ```

4. Add `bazel` to the `$PATH`.
   ```sh
   export PATH=<my-user-dir-on-local-disk>/bin:$PATH
   bazel --version # Should be "bazel 4.0.0"
   ```

### Compiler choice
Be default our build system configures Bazel to use Intel(R) C++ Compiler
in case of normal C++ code and Intel(R) oneAPI DPC++ Compiler in case of
DPC++. If Intel(R) C++ Compiler is not available in the `$PATH`, Bazel
tries to find default compiler for specific OS, e.g., GCC for Linux.

The C++ compiler can be forcibly changed using environment variable `CC`.
```sh
export CC=gcc
bazel <bazel-command> ... # Will use GCC for normal C++ code
```

## Common Bazel commands
The most used Bazel commands are `build`, `test` and `run`.
- `build` builds specified target.
  ```sh
  bazel build <target-name>
  ```

  Target names are always bound to the location in repository
  and defined in `BUILD` files. For example, test for `dal::array`
  is defined in `<repo-root>/cpp/oneapi/dal/BUILD`, so the name of
  the Bazel target will be `//cpp/oneapi/dal:array_test`.
  Here `//` stands for repository root.

  For simplicity, you can use relative target names.
  For example, if you are already in the repository root,
  you can simply use `cpp/oneapi/dal:array_test`.

- `run` builds and runs specified target
  ```sh
  bazel run <target-name>
  ```

  > Note: There is no need to build and run targets separately,
  `run` builds if needed.

  > Note: `run` cannot be used to run multiple targets at once,
  consider use of `test`.

- `test` builds and runs specified test target
  ```sh
  bazel test <test-target-name>
  ```

  There is no syntax difference between in targets and test targets,
  but test targets may contain a set of multiple executables
  called *test suite*.

- `clean` removes built artifacts and cleans the cache.
  ```sh
  bazel clean
  ```

  When you update Bazel version, want to shutdown Bazel server or suspect that
  something went wrong it is recommended to add `--expunge` option.
  ```sh
  bazel clean --expunge
  ```

## Bazel options
- `--compilation_mode [-c]` Bazel comes with three compilation modes `opt`,
  `dbg` and `fastbuild`. \
  Possible values:
   - `opt` _(default)_ compiles everything with optimizations `-O2`.
   - `dbg` enables `-g`, `-O0` compiler switches and **enables assertions**.
   - `fastbuild` optimizes build time, no optimizations, no debug information.
     Useful when one introduces massive changes and wants to check whether they
     break the build.

   Example:
   ```sh
   bazel test -c dbg //cpp/oneapi/dal:tests
   ```

   It's highly encouraged to run newly added tests on the local machine with
   `-c dbg` option to test assertions.

- `--config` Takes effect only for `run` or `test` commands. Specifies
  interface that is used to build and run tests. \
  Possible values:
  - `<not specified>` _(default)_ Build and run all tests.
  - `host` Build and run tests for HOST interface without dependencies on DPC++
    runtime. Uses regular C++ compiler.
  - `dpc` Build and run tests for DPC++ interface. Uses Intel(R) DPC++ Compiler.
  - `host-public` Build and run tests only for public part of C++ interfaces.
  - `dpc-public` Build and run tests only for public part of DPC++ interfaces.

   Example:
   ```sh
   bazel test --config=host //cpp/oneapi/dal:tests
   ```

- `--device` Takes effect only for `run` or `test` commands and option
  `--config` that implies build of DPC++ interfaces. Specifies device to create
  a queue and run tests. \
  Possible values:
  - `auto` _(default)_ Automatically detects available device. Tries to detect
    GPU first, if no GPUs available tries to use CPU. If no device runtimes
    found, uses `host_selector`.
  - `cpu` Creates queue using `cpu_selector`.
  - `gpu` Creates queue using `gpu_selector`.

   Example:
   ```sh
   bazel test --config=dpc --device=gpu //cpp/oneapi/dal:tests
   ```

- `--cpu` CPU instruction sets to compile library for. \
  Possible values:
  - `auto` _(default)_ Automatically detects highest available instruction set
    on the local machine. If detection failed, uses `avx2`.
  - `modern` Compiles for `sse2`, `avx`, `avx2`, `avx512`.
  - `all` Compiles for all instruction sets listed below.
  - Any comma-separated combination of the following values:
    - `sse2`
    - `ssse3`
    - `sse42`
    - `avx`
    - `avx2`
    - `avx512`
    - `avx512_mic`

   Example:
   ```sh
   bazel test --cpu="avx,avx512" //cpp/oneapi/dal:tests
   ```

- `--test_external_datasets` TBD
- `--test_nightly` TBD
- `--test_weekly` TBD
- `--test_link_mode` TBD
- `--test_thread_mode` TBD


## Build recipes for oneDAL
### Run oneAPI examples
- To run all oneAPI C++ example use the following commands:
  ```sh
  bazel test //examples/oneapi/cpp:all
  ```

- To run all oneAPI DPC++ examples ... It's not implemented yet...

### Run oneAPI tests
- To run all test use the following commands:
  ```sh
  bazel test //cpp/oneapi/dal:tests
  ```

  This will run all tests including HOST and DPC++ interfaces. In case of DPC++
  tests, queue will be created depending machine configuration. If there is a
  GPU, `sycl::gpu_selector` is used, otherwise queue is created with
  `sycl::cpu_selector`.

- To run all HOST tests use the following commands:
  ```sh
  bazel test --config=host //cpp/oneapi/dal:tests
  ```

- To run all DPC++ tests use the following commands:
  ```sh
  bazel test --config=dpc //cpp/oneapi/dal:tests
  ```

  If you need run DPC++ tests on specific device use the `device` option:
  ```sh
  bazel test --config=dpc --device=gpu //cpp/oneapi/dal:tests
  ```

- To run specific **set of tests**, e.g., for specific algorithm:
  ```sh
  bazel test //cpp/oneapi/dal/algo/svm:tests
  ```

  This will run all tests for SVM algorithm including HOST and DPC++ interfaces.
  To control type of interfaces and target device, use the `--config` and
  `--device` options described above.

- To run specific **test** and see output in stdout:
  ```sh
  # Use HOST interface to build and run the test
  bazel run //cpp/oneapi/dal/algo/pca:test_batch_host

  # Use DPC++ interface to build and run the test
  bazel run //cpp/oneapi/dal/algo/pca:test_batch_dpc
  ```

  This is useful when one tests is failed on `bazel test` and you want to focus
  on one particular test to fix the problem. The name of the test can be taken
  from `bazel test //cpp/oneapi/dal:tests` output as it prints names of all test
  targets.

- You can run test executables produced by Bazel manually. The compiled
  executables are stored in corresponding `bazel-bin` subdirectory. For example,
  if the test target name is `//cpp/oneapi/dal/pca:test_batch_host`, the
  executable `test_batch_host` will be stored in
  `<repo-root>/bazel-bin/cpp/oneapi/dal/algo/pca` directory.

## Advanced testing
### Tests that use external datasets
Some tests may depend on quite large datasets that cannot be stored in the Git
repository. In that case test should read dataset from `$DAAL_DATASETS`
directory that could point to datasets storage on local disk. We call such
datasets _external_.

Such test cases are disabled by default, but can be enabled using
`--test_external_datasets` switch.
```sh
bazel test --test_external_datasets //cpp/oneapi/dal:tests
```

Test cases with external datasets must be marked by `[external-dataset]` tag,
which is used to filter out tests at runtime if no `--test_external_datasets`
provided.
```c++
TEST("my test that uses external dataset", "[external-dataset]") {
    // Code that reads some external dataset
}
```

### Nightly tests
Nightly tests are disabled by default. The `--test_nightly` option enables tests
marked with `[nightly]` tags.
```sh
bazel test --test_nightly //cpp/oneapi/dal:tests
```

Mark nightly test cases with `[nightly]` tag:
```c++
TEST("my test that takes long time", "[nightly]") { /* ... */ }
```

### Weekly tests
Weekly tests are disabled by default. The `--test_weekly` option enables tests
marked with `[weekly]` or `[nightly]` tags. **Weekly tests include nightly.**
```sh
bazel test --test_weekly //cpp/oneapi/dal:tests
```

Mark weekly test cases with `[weekly]` tag:
```c++
TEST("my test that takes very long time", "[weekly]") { /* ... */ }
```

### Tests for public interface
Sometimes it is necessary to build and run only part of tests responsible for
public interface testing, for example, to test backward compatibility. There is
the `host-public` config designed for that use case.
```sh
bazel test --config=host-public //cpp/oneapi/dal:tests
```

Test is categorized to public or private using the `private` flag of the
`dal_tst_suite` rule. By default, all test suites are public, if the new test
suite for the private functionality is defined, the flag should be turned on.
```py
dal_test_suite(
    name = "my_tests",
    private = True,
    ...
)
```

### Run tests for the existing oneDAL build
TBD


## What is missing in this guide
- Choice of threading layer
- How to get make-like release structure
