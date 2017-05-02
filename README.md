# jTox Jolla Tox Client

jTox is a Jolla/SailfishOS [https://tox.chat](Tox) client.

### ![Danger: Experimental](https://github.com/TokTok/c-toxcore/raw/master/other/tox-warning.png)

jTox uses an **experimental** cryptographic network [library](https://github.com/TokTok/c-toxcore).
It has not been
formally audited by an independent third party that specializes in
cryptography or cryptanalysis. The protocol is not optimized for mobile yet
and causes significant data traffic. **Use at your own risk.**

## Building toxcore for Jolla

### Requirements

These steps will install all the required packages on mersdk build engine.
Be sure to log into the engine as **mersdk** user not as root.

`sb2 -t SailfishOS-armv7hl -m sdk-install -R zypper install gcc gcc-c++ autoconf automake make libtool`

`sb2 -t SailfishOS-i486 -m sdk-install -R zypper install gcc gcc-c++ autoconf automake make libtool`

### Building the required libraries

#### Using the populate script

**only for linux**

The populate script should just work(tm) if you're running it on Linux from the
`support` directory and your SailfishOS SDK install path is standard.

Just run it with `./populate.sh` and everything should work. If not see next
section

#### Manually using the build script

Provided build script in `support/build.sh` needs to be copied to a subdirectory
on the build engine. Use the `mersdk` user, do not use root.

Once in a new directory, say run the script `./build.sh` with target architecture
as parameter. Valid architectures are `i486` and `armv7hl`.

    `./build.sh i486`
    `./build.sh armv7hl`

It will download latest versions of `libsodium` and `c-toxcore` libraries from
github and build them.
Once done the resulting libraries are in the `libs` folder
while the include files are in the `include` folder of the fake root directory
for given architecture e.g. `i486/usr/lib`.

You then need to copy over the required files to `extra` so your resulting
structure in the project should be `extra/i486/...` and `extra/armv7hl/...`

Bin subdirectory is not required and neither are any non `.a` files in the lib
directory.
