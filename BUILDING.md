# Building nsasm

## Installing clang-9

Because I am a C++ nerd, I use C++17 features, rather than targeting the
version of the language that is widely deployed.  You need to install a modern
compiler to build nsasm.


1. Open a shell, and determine your ubuntu version number.

```
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 18.04.2 LTS
Release:        18.04
Codename:       bionic
```

2. Visit https://apt.llvm.org/, and find the link to the package repository
which has nightly versions of clang.

3. Add clang's signature to the apt-get keychain

`$ wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -`

4. Add the appropriate repository for your version of Ubuntu, replacing
the quoted portion of the following as appropriate

```
$ sudo add-apt-repository \
  "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic main"
```

5. Install clang-9

`$ sudo apt-get install clang-9 lldb-9 lld-9 libc++-9-dev`

6. Set your CC environment variable to point to clang-9.  This is how bazel
knows which compiler to use.

`export CC=/usr/bin/clang-9`

You'll want to add this line to your `.bashrc` or equivalent if you intend to
build nsasm regularly.

## Installing bazel

I use bazel, sorry.  Installing from the binary installer is easiest, and
works for me.  This installs the binary for a single user and does not require
root.

The instructions on https://docs.bazel.build/versions/master/install-ubuntu.html are straightforward.  (You'll want the `linux-x86_64.sh` installer.)

## Checking out nsasm

Create a directory for nsasm development and clone the repo.  If you just want
to get started quick and dirty, run

    $ git clone https://github.com/vslashg/nsasm.git

You may have to `apt-get install` git first.


## building nsasm

bazel's options story is absurd; there's no good way to specify project-wide
copts.  Until I fix this in the build files, you can run all tests and build
all tools by executing the following from the directory you just pulled from
git:

    $ bazel test --copt --std=c++17 //...
    $ bazel build --copt --std=c++17 //...
