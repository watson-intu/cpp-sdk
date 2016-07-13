# Watson Developer Cloud C++ SDK
This SDK abstract serveral of the services available through Bluemix.

# Building

NOTE: It probably is not necessary to download the toolchains from the aldebaran website to compile the WDC SDK.

## Building for Nao/Pepper using OSX

1. Setup qibuild & cmake, see http://doc.aldebaran.com/2-1/dev/cpp/install_guide.html for instructions on getting those installed.
2. Download the "Cross Toolchain" for Mac from https://community.aldebaran.com/en/resources/software and unzip into ~/toolchains/ctc-mac64-atom.2.4.2.26/.
3. Run the following commmands:
  * cd {self root directory}
  * qitoolchain create pepper ~/toolchains/ctc-mac64-atom.2.4.2.26/toolchain.xml
  * qitoolchain add-package -c pepper packages/openssl-i686-aldebaran-linux-gnu-1.0.1s.zip
  * qibuild init
  * qibuild add-config pepper --toolchain pepper --default
  * qibuild configure
  * qibuild make

## Building for Windows

1. Install Visual Studio 2015.
2. Open the solution found in vs2015/wdc-sdk.sln

## Building for OSX

1. Setup qibuild and CMake. You can use Homebrew to install CMake, and any distribution of Python (2.7 recommended) to install qibuild through pip.
2. Download the "Mac Toolchain" for Mac (C++ SDK 2.1.4 Mac 64) from https://community.aldebaran.com/en/resources/software and unzip into ~/toolchains/naoqi-sdk-mac64/.
3. Run the following commands:
  * cd {self root directory}
  * qitoolchain create mac ~/toolchains/naoqi-sdk-mac64/toolchain.xml
  * qibuild init
  * qibuild add-config mac --toolchain mac
  * qibuild configure -c mac
  * qibuild make -c mac

## Building for Linux

1. Setup qibuild and CMake. You can use your Linux package manager to install CMake, and any distribution of Python (2.7 recommended) to install qibuild through pip.
2. Download the "Linux Toolchain" for Linux (C++ SDK 2.1.4 Linux 64) from https://community.aldebaran.com/en/resources/software and unzip into ~/toolchains/naoqi-sdk-linux64/.
3. Run the following commands:
  * cd {self root directory}
  * qitoolchain create linux ~/toolchains/naoqi-sdk-linux64/toolchain.xml
  * qibuild init
  * qibuild add-config linux --toolchain linux
  * qibuild configure -c linux
  * qibuild make -c linux

