# Building nsasm

## Installing clang-9

I use clang-9 for development, but nsasm should work on any reasonably up-to-date
compiler.  If you still want to try installing clang-9, do the following:

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

I gave up on trying to use C++17 features in nsasm, which makes the story rather
straightforward.

    $ bazel test //...
    $ bazel build //...
