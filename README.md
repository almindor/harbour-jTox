# jTox Jolla Tox Client

jTox is a Jolla/SailfishOS [https://tox.chat](Tox) client.

### ![Danger: Experimental](https://github.com/TokTok/c-toxcore/raw/master/other/tox-warning.png)

jTox uses an **experimental** cryptographic network [library](https://github.com/TokTok/c-toxcore). It has not been formally audited by an independent third party that specializes in cryptography or cryptanalysis. The protocol is not optimized for mobile yet and causes significant data traffic. **Use at your own risk.**

## Community

[![Gitter](https://badges.gitter.im/almindor/jtox.svg)](https://gitter.im/almindor/jtox?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

## Building toxcore for Jolla

Increasing the VM RAM and CPU count inside VirtualBox for the build engine is highly recommended.

### Requirements

These steps will install all the required packages on mersdk build engine.
See [the sailfish SDK FAQ](https://sailfishos.org/develop/sdk-overview/develop-faq/) for instructions on how to connect to the build engine.
Be sure to log into the engine as **mersdk** user not as root.

Change this to the SailfishOS version required, also in **support/build.sh**

`export SFVER='3.2.0.12'`

`sb2 -t SailfishOS-$SFVER-armv7hl -m sdk-install -R zypper install gcc gcc-c++ autoconf automake make libtool nasm`

`sb2 -t "SailfishOS-$SFVER-i486" -m sdk-install -R zypper install gcc gcc-c++ autoconf automake make libtool yasm`

### Building the required libraries

Make sure the build VM is running (the one from SailfishOS IDE).

To improve performance enable multiple cores for the build VM in VirtualBox.

#### Using the populate script

**only for linux**

The populate script should just work(tm) if you're running it on Linux from the `support` directory and your SailfishOS SDK install path is standard.

NOTE: you can edit `build.sh` and change the `SFVER`, `SODIUMVER`, `TOXCOREVER` and `THREADS` as needed.

Just run it with `./populate.sh` and everything should work. Change SailfishOS version in the env var as required. If not see next section

#### Manually using the build script

Provided build script in `support/build.sh` needs to be copied to a subdirectory on the build engine. Use the `mersdk` user, do not use root.

Once in a new directory, say run the script `./build.sh` with target architecture as parameter. Valid architectures are `i486` and `armv7hl`. You can also edit the `build.sh` script and change the required variables (see above).

    `./build.sh i486`
    `./build.sh armv7hl`

It will download latest versions of `libsodium` and `c-toxcore` libraries from github and build them. Once done the resulting libraries are in the `libs` folder while the include files are in the `include` folder of the fake root directory for given architecture e.g. `i486/usr/lib`.

You then need to copy over the required files to `extra` so your resulting structure in the project should be `extra/i486/...` and `extra/armv7hl/...`

Bin subdirectory is not required and neither are any non `.a` files in the lib directory.
